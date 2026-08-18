#pragma once

#include <string>

class Settings {
public:
    Settings(const std::string& nameSpace, bool writable);
    ~Settings();

    int GetInt(const std::string& key, int defaultValue) const;
    void SetInt(const std::string& key, int value);

private:
    class QSettingsHolder;
    QSettingsHolder* _settings;
};