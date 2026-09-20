/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "ZeroconfResolver.h"

#include <QtCore/QSocketNotifier>
#include <QtEndian>

ZeroconfResolver::ZeroconfResolver(const ZeroconfRecord& record, QObject* parent) :
    QObject(parent), record_(record)
{
}

ZeroconfResolver::~ZeroconfResolver()
{
    if (ref_) {
        DNSServiceRefDeallocate(ref_);
    }
}

void ZeroconfResolver::start()
{
    const auto errorCode = DNSServiceResolve(
        &ref_, 0, 0, record_.serviceName.toUtf8().constData(),
        record_.registeredType.toUtf8().constData(),
        record_.replyDomain.toUtf8().constData(), resolveReply, this);
    if (errorCode != kDNSServiceErr_NoError) {
        Q_EMIT error(errorCode);
        return;
    }
    const auto socket = DNSServiceRefSockFD(ref_);
    if (socket < 0) {
        Q_EMIT error(kDNSServiceErr_Invalid);
        return;
    }
    socket_ = std::make_unique<QSocketNotifier>(socket, QSocketNotifier::Read, this);
    connect(socket_.get(), &QSocketNotifier::activated,
            this, &ZeroconfResolver::socketReadyRead);
}

void ZeroconfResolver::socketReadyRead()
{
    const auto errorCode = DNSServiceProcessResult(ref_);
    if (errorCode != kDNSServiceErr_NoError) {
        Q_EMIT error(errorCode);
    }
}

void ZeroconfResolver::resolveReply(DNSServiceRef, DNSServiceFlags, uint32_t,
    DNSServiceErrorType errorCode, const char*, const char* hosttarget,
    uint16_t port, uint16_t txtLen, const unsigned char* txtRecord, void* context)
{
    auto* resolver = static_cast<ZeroconfResolver*>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        Q_EMIT resolver->error(errorCode);
        return;
    }

    resolver->record_.hostName = QString::fromUtf8(hosttarget);
    resolver->record_.port = qFromBigEndian(port);
    const auto itemCount = TXTRecordGetCount(txtLen, txtRecord);
    for (uint16_t index = 0; index < itemCount; ++index) {
        char key[256] = {};
        uint8_t valueLength = 0;
        const void* value = nullptr;
        if (TXTRecordGetItemAtIndex(txtLen, txtRecord, index, sizeof(key), key,
                                    &valueLength, &value) == kDNSServiceErr_NoError) {
            resolver->record_.txtRecords.insert(
                QString::fromUtf8(key),
                QString::fromUtf8(static_cast<const char*>(value), valueLength));
        }
    }
    Q_EMIT resolver->resolved(resolver->record_);
    resolver->deleteLater();
}
