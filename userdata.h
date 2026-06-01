#ifndef USERDATA_H
#define USERDATA_H

#include <QString>

struct UserSettings {
    QString avatarPath;
    QString name = "用户";
    QString gender;
    int height = 0;   // cm
    int weight = 0;   // kg
    int age = 0;
    int bgmVolume = 50;  // 0-100
};

class UserData {
public:
    bool load(const QString &filePath);
    bool save(const QString &filePath) const;

    UserSettings settings;
};

#endif
