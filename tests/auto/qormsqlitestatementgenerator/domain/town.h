#pragma once

#include <QObject>

class Province;

class Town : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Province* province READ province WRITE setProvince NOTIFY provinceChanged)

    int m_id;
    QString m_name;
    Province* m_province = nullptr;

public:
    Q_INVOKABLE explicit Town(QObject* parent = nullptr);
    Town(int id, const QString& name, Province* province)
        : m_id{id}
        , m_name{name}
        , m_province{province}
    {
    }

    Town(const QString& name, Province* province)
        : m_name{name}
        , m_province{province}
    {
    }

    int id() const;
    void setId(int id);

    QString name() const;
    void setName(QString name);

    Province* province() const;
    void setProvince(Province* province);

signals:
    void idChanged(int id);
    void nameChanged(QString name);
    void provinceChanged(Province* province);
};
