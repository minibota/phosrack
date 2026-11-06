// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QSettings>
class Preferences : public QObject {
    Q_OBJECT
public:
    explicit Preferences(QObject *parent=nullptr):QObject(parent),settings("Phosrack","Phosrack"){}
    Q_INVOKABLE QVariant read(const QString &key,const QVariant &fallback={}) {return qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")?fallback:settings.value("ui/"+key,fallback);}
    Q_INVOKABLE void write(const QString &key,const QVariant &value) {if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))settings.setValue("ui/"+key,value);}
private:
    QSettings settings;
};
