/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QMap>

class ZeroconfRecord
{
public:
    ZeroconfRecord() {}
    ZeroconfRecord(const QString& name, const QString& regType,
                   const QString& domain, const QString& host = {},
                   quint16 servicePort = 0,
                   const QMap<QString, QString>& attributes = {})
        : serviceName(name), registeredType(regType), replyDomain(domain),
          hostName(host), port(servicePort), txtRecords(attributes)
    {}
    ZeroconfRecord(const char* name, const char* regType, const char* domain)
    {
        serviceName = QString::fromUtf8(name);
        registeredType = QString::fromUtf8(regType);
        replyDomain = QString::fromUtf8(domain);
    }

    bool operator==(const ZeroconfRecord& other) const {
        return serviceName == other.serviceName
            && registeredType == other.registeredType
            && replyDomain == other.replyDomain;
    }

public:
    QString serviceName;
    QString registeredType;
    QString replyDomain;
    QString hostName;
    quint16 port{0};
    QMap<QString, QString> txtRecords;
};

Q_DECLARE_METATYPE(ZeroconfRecord)
