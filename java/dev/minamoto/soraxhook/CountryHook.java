package dev.minamoto.soraxhook;

/** Callback contract required by LSPlant: public Object method(Object[]). */
public final class CountryHook {
    private final String country;

    public CountryHook(String country) {
        this.country = country;
    }

    public Object replace(Object[] args) {
        return country;
    }
}
