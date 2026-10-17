#include <sys/types.h>
#include "zygisk.hpp"
#include "config.hpp"
#include "art_symbols.hpp"
#include "hook_dex.hpp"
#include <android/log.h>
#include <dobby.h>
#include <lsplant.hpp>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "SoraXHook", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SoraXHook", __VA_ARGS__)

namespace {
using namespace soraxhook;
struct Reply { char country[2] = {'u', 's'}; unsigned char status = 0; };
static_assert(sizeof(Reply) == 3);

bool Transfer(int fd, void* buffer, size_t length, bool sending) {
    auto* p = static_cast<char*>(buffer);
    while (length) {
        pollfd item{fd, static_cast<short>(sending ? POLLOUT : POLLIN), 0};
        int ready;
        do { ready = poll(&item, 1, 1500); } while (ready < 0 && errno == EINTR);
        if (ready <= 0) return false;
        const ssize_t count = sending ? send(fd, p, length, MSG_NOSIGNAL) : recv(fd, p, length, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        p += count;
        length -= static_cast<size_t>(count);
    }
    return true;
}

void Companion(int socket) {
    // /sdcard may not be mounted in the companion's namespace. Both alternatives
    // refer to the owner's same Documents directory, not separate configurations.
    constexpr std::array paths{
        kConfigPath,
        "/storage/emulated/0/Documents/MinamotoDev/SoraXHook/config.yaml",
        "/data/media/0/Documents/MinamotoDev/SoraXHook/config.yaml"};
    Reply reply;
    int fd = -1;
    for (const char* path : paths) {
        fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        if (fd >= 0) break;
    }
    if (fd >= 0) {
        std::string content;
        std::array<char, 1024> buffer{};
        bool read_error = false;
        while (content.size() <= kMaxConfigBytes) {
            const ssize_t n = read(fd, buffer.data(), buffer.size());
            if (n < 0 && errno == EINTR) continue;
            if (n < 0) { read_error = true; break; }
            if (!n) break;
            content.append(buffer.data(), static_cast<size_t>(n));
        }
        close(fd);
        const Config config = ParseConfig(content);
        if (config.valid && !read_error) {
            reply.country[0] = config.country[0];
            reply.country[1] = config.country[1];
            reply.status = 1;
        } else {
            reply.status = 2;
        }
    }
    Transfer(socket, &reply, sizeof(reply), true);
}

bool ClearException(JNIEnv* env, const char* stage) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionClear();
    LOGE("JNI failed: %s", stage);
    return true;
}

bool Install(JNIEnv* env, const std::string& country) {
    // Resolve the callback before initializing ART hooks. The embedded dex buffer
    // and module must remain loaded throughout the lifetime of the hooked process.
    const jclass cl = env->FindClass("java/lang/ClassLoader");
    if (ClearException(env, "ClassLoader") || !cl) return false;
    const jmethodID get_system = env->GetStaticMethodID(cl, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    if (ClearException(env, "getSystemClassLoader") || !get_system) return false;
    const jobject parent = env->CallStaticObjectMethod(cl, get_system);
    if (ClearException(env, "system class loader")) return false;
    const jobject bytes = env->NewDirectByteBuffer(const_cast<unsigned char*>(kHookDex), sizeof(kHookDex));
    if (ClearException(env, "dex buffer") || !bytes) return false;
    const jclass dex_loader = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    if (ClearException(env, "InMemoryDexClassLoader") || !dex_loader) return false;
    const jmethodID loader_ctor = env->GetMethodID(dex_loader, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    if (ClearException(env, "dex loader constructor") || !loader_ctor) return false;
    const jobject loader = env->NewObject(dex_loader, loader_ctor, bytes, parent);
    if (ClearException(env, "dex load") || !loader) return false;
    const jmethodID load_class = env->GetMethodID(cl, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (ClearException(env, "loadClass") || !load_class) return false;
    const jstring callback_name = env->NewStringUTF("dev.minamoto.soraxhook.CountryHook");
    if (ClearException(env, "callback name") || !callback_name) return false;
    const auto callback_class = static_cast<jclass>(env->CallObjectMethod(loader, load_class, callback_name));
    if (ClearException(env, "callback class") || !callback_class) return false;
    const jmethodID ctor = env->GetMethodID(callback_class, "<init>", "(Ljava/lang/String;)V");
    if (ClearException(env, "callback constructor") || !ctor) return false;
    const jmethodID callback_id = env->GetMethodID(callback_class, "replace", "([Ljava/lang/Object;)Ljava/lang/Object;");
    if (ClearException(env, "callback method") || !callback_id) return false;
    const jstring code = env->NewStringUTF(country.c_str());
    if (ClearException(env, "country string") || !code) return false;
    const jobject hooker = env->NewObject(callback_class, ctor, code);
    if (ClearException(env, "callback instance") || !hooker) return false;
    const jclass telephony = env->FindClass("android/telephony/TelephonyManager");
    if (ClearException(env, "TelephonyManager") || !telephony) return false;
    const jmethodID target_id = env->GetMethodID(telephony, "getNetworkCountryIso", "()Ljava/lang/String;");
    if (ClearException(env, "getNetworkCountryIso()") || !target_id) return false;
    const jobject target = env->ToReflectedMethod(telephony, target_id, JNI_FALSE);
    if (ClearException(env, "target reflection") || !target) return false;
    const jobject callback = env->ToReflectedMethod(callback_class, callback_id, JNI_FALSE);
    if (ClearException(env, "callback reflection") || !callback) return false;
    ArtSymbols art;
    if (!art.valid()) { LOGE("Cannot read loaded libart ELF"); return false; }
    LOGI("ART compressed symbols: %s", art.has_debug_data() ? "decoded" : "absent or unreadable");
    lsplant::InitInfo info{};
    info.inline_hooker = [](void* address, void* replacement) -> void* {
            dobby_dummy_func_t backup = nullptr;
            return DobbyHook(address, reinterpret_cast<dobby_dummy_func_t>(replacement), &backup) == 0
                ? reinterpret_cast<void*>(backup) : nullptr;
        };
    info.inline_unhooker = [](void* address) { return DobbyDestroy(address) == 0; };
    info.art_symbol_resolver = [&art](std::string_view name) { return art.Find(name); };
    info.art_symbol_prefix_resolver = [&art](std::string_view name) { return art.Find(name, true); };
    if (!lsplant::Init(env, info)) {
        ClearException(env, "LSPlant init");
        LOGE("LSPlant initialization failed");
        return false;
    }
    const jobject backup = lsplant::Hook(env, target, hooker, callback);
    if (ClearException(env, "LSPlant hook") || !backup) { LOGE("Hook installation failed"); return false; }
    // LSPlant retains the hook context; retain the loader and backup explicitly too.
    static jobject loader_ref = env->NewGlobalRef(loader);
    if (ClearException(env, "loader reference") || !loader_ref) return false;
    static jobject backup_ref = env->NewGlobalRef(backup);
    if (ClearException(env, "backup reference") || !backup_ref) return false;
    LOGI("Installed getNetworkCountryIso() -> %s", country.c_str());
    // Probe a receiver in this target process. The replacement never reads its
    // fields, so this avoids needing an Activity/Context before app startup.
    const jobject receiver = env->AllocObject(telephony);
    if (ClearException(env, "self-check receiver") || !receiver) return true;
    const auto actual = static_cast<jstring>(env->CallObjectMethod(receiver, target_id));
    if (ClearException(env, "self-check invocation") || !actual) {
        LOGE("Self-check returned no country");
        return true;
    }
    const char* actual_text = env->GetStringUTFChars(actual, nullptr);
    if (ClearException(env, "self-check result") || !actual_text) return true;
    if (country == actual_text) LOGI("Self-check getNetworkCountryIso() -> %s: PASS", actual_text);
    else LOGE("Self-check country mismatch: expected=%s actual=%s", country.c_str(), actual_text);
    env->ReleaseStringUTFChars(actual, actual_text);
    return true;
}

class Module final : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override { api_ = api; env_ = env; }
    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (!args->nice_name || !args->app_data_dir) { Unload(); return; }
        const char* process = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (ClearException(env_, "process name") || !process) { Unload(); return; }
        const char* dir = env_->GetStringUTFChars(args->app_data_dir, nullptr);
        if (!ClearException(env_, "app data dir") && dir) {
            target_ = IsTarget(process, dir);
            env_->ReleaseStringUTFChars(args->app_data_dir, dir);
        }
        env_->ReleaseStringUTFChars(args->nice_name, process);
        if (!target_) { Unload(); return; }
        LOGI("Target process selected: pid=%d", getpid());
        const int socket = api_->connectCompanion();
        Reply reply;
        if (socket >= 0) {
            if (Transfer(socket, &reply, sizeof(reply), false) &&
                reply.country[0] >= 'a' && reply.country[0] <= 'z' &&
                reply.country[1] >= 'a' && reply.country[1] <= 'z') {
                country_.assign(reply.country, 2);
                if (reply.status == 2) LOGE("Invalid/unreadable config; using us");
            } else { LOGE("Config IPC failed; using us"); }
            close(socket);
        } else { LOGE("Companion unavailable; using us"); }
    }
    void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
        if (!target_) return;
        if (env_->PushLocalFrame(64) < 0) { ClearException(env_, "local frame"); return; }
        Install(env_, country_);
        env_->PopLocalFrame(nullptr);
        // Do not unload: LSPlant/Dobby callbacks reside in this shared library,
        // including partially initialized hooks after an initialization failure.
    }
    void preServerSpecialize(zygisk::ServerSpecializeArgs*) override { Unload(); }
private:
    void Unload() { api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY); }
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    bool target_ = false;
    std::string country_ = "us";
};
}
REGISTER_ZYGISK_MODULE(Module)
REGISTER_ZYGISK_COMPANION(Companion)
