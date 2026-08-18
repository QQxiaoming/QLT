#include "settings.h"

#include <QSettings>
#include <QStandardPaths>

class Settings::QSettingsHolder {
public:
    explicit QSettingsHolder(const std::string& nameSpace)
        : settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/settings.ini",
                   QSettings::IniFormat)
    {
        settings.beginGroup(QString::fromStdString(nameSpace));
    }

    ~QSettingsHolder()
    {
        settings.endGroup();
        settings.sync();
    }

    QSettings settings;
};

Settings::Settings(const std::string& nameSpace, bool writable) : _settings(new QSettingsHolder(nameSpace))
{
    Q_UNUSED(writable);
}

Settings::~Settings()
{
    delete _settings;
}

int Settings::GetInt(const std::string& key, int defaultValue) const
{
    return _settings->settings.value(QString::fromStdString(key), defaultValue).toInt();
}

void Settings::SetInt(const std::string& key, int value)
{
    _settings->settings.setValue(QString::fromStdString(key), value);
}