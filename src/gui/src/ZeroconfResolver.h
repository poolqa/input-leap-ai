/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include "ZeroconfRecord.h"

#include <QtCore/QObject>

#define _MSL_STDINT_H
#include <stdint.h>
#include <dns_sd.h>

#include <memory>

class QSocketNotifier;

class ZeroconfResolver : public QObject
{
    Q_OBJECT

public:
    explicit ZeroconfResolver(const ZeroconfRecord& record, QObject* parent = nullptr);
    ~ZeroconfResolver() override;
    void start();

Q_SIGNALS:
    void resolved(const ZeroconfRecord& record);
    void error(DNSServiceErrorType error);

private slots:
    void socketReadyRead();

private:
    static void DNSSD_API resolveReply(DNSServiceRef, DNSServiceFlags, uint32_t,
        DNSServiceErrorType, const char*, const char*, uint16_t, uint16_t,
        const unsigned char*, void*);

    ZeroconfRecord record_;
    DNSServiceRef ref_{nullptr};
    std::unique_ptr<QSocketNotifier> socket_;
};
