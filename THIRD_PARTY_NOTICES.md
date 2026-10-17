# Third-party components

* LSPlant — https://github.com/LSPosed/LSPlant
  revision d8b5d1dbb664abc606644036822e4bb64547edf6, LGPL-3.0.
  Includes its pinned DexBuilder / parallel-hashmap dependencies.
* Dobby — https://github.com/jmpews/Dobby
  revision 21f07a5ce184cda9346afb8009c07fe7e47390d0, Apache-2.0.
* Zygisk public API header — https://github.com/topjohnwu/zygisk-module-sample
  revision 8ce26128f81baaed0b969aaf7f52f886b61af4ab, ISC license in the header.
* XZ Embedded — https://github.com/tukaani-project/xz-embedded
  revision ae63ae3a36ed01724674e8f3d750dc47bf125410, 0BSD.

Dependencies are compiled from source. LSPlant and Dobby are linked statically.
The build script emits a corresponding source archive alongside the binary ZIP;
distribute them together. See each dependency's included license and notices.
