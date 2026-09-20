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

#include "ZeroconfService.h"

#include "MainWindow.h"
#include "ZeroconfRegister.h"
#include "ZeroconfBrowser.h"
#include "ZeroconfResolver.h"
#include "common/DataDirectories.h"
#include "inputleap/PeerConfig.h"
#include "net/FingerprintDatabase.h"

#include <QtNetwork>
#include <QMessageBox>
#define _MSL_STDINT_H
#include <stdint.h>
#include <dns_sd.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <stdlib.h>
#endif

static const QStringList preferedIPAddress(
                QStringList() <<
                "192.168." <<
                "10." <<
                "172.");

const char* ZeroconfService:: m_ServerServiceName = "_inputLeapServerZeroconf._tcp";
const char* ZeroconfService:: m_ClientServiceName = "_inputLeapClientZeroconf._tcp";
const char* ZeroconfService:: m_PeerServiceName = "_inputleap-peer._tcp";

static void silence_avahi_warning()
{
    // the libavahi folks seemingly find Apple's bonjour API distasteful
    // and are quite liberal in taking it out on users...unless we set
    // this environmental variable before calling the avahi library.
    // additionally, Microsoft does not give us a POSIX setenv() so
    // we use their OS-specific API instead
    const char *name  = "AVAHI_COMPAT_NOWARN";
    const char *value = "1";
#ifdef _WIN32
#if QT_VERSION_MAJOR < 6
    SetEnvironmentVariable(name, value);
#else
    SetEnvironmentVariable(reinterpret_cast<LPCWSTR>(name), reinterpret_cast<LPCWSTR>(value));
#endif
#else
    setenv(name, value, 1);
#endif
}

ZeroconfService::ZeroconfService(MainWindow* mainWindow) :
    m_pMainWindow(mainWindow),
    m_ServiceRegistered(false)
{
    silence_avahi_warning();
    if (m_pMainWindow->app_role() == AppRole::Server) {
        if (registerService(true)) {
            zeroconf_browser_ = std::make_unique<ZeroconfBrowser>(this);
            connect(zeroconf_browser_.get(), &ZeroconfBrowser::currentRecordsChanged, this, &ZeroconfService::clientDetected);
            zeroconf_browser_->browseForType(QLatin1String(m_ClientServiceName));
        }
    }
    else {
        zeroconf_browser_ = std::make_unique<ZeroconfBrowser>(this);
        connect(zeroconf_browser_.get(), &ZeroconfBrowser::currentRecordsChanged, this, &ZeroconfService::serverDetected);
        zeroconf_browser_->browseForType(QLatin1String(m_ServerServiceName));
    }

    if (zeroconf_browser_) {
        connect(zeroconf_browser_.get(), &ZeroconfBrowser::error,
                this, &ZeroconfService::errorHandle);
    }

    if (m_pMainWindow->peerModeEnabled() && registerPeerService()) {
        peer_browser_ = std::make_unique<ZeroconfBrowser>(this);
        connect(peer_browser_.get(), &ZeroconfBrowser::currentRecordsChanged,
                this, &ZeroconfService::peerDetected);
        connect(peer_browser_.get(), &ZeroconfBrowser::error,
                this, &ZeroconfService::errorHandle);
        peer_browser_->browseForType(QLatin1String(m_PeerServiceName));
    }
}

ZeroconfService::~ZeroconfService() = default;

void ZeroconfService::serverDetected(const QList<ZeroconfRecord>& list)
{
    for (const ZeroconfRecord& record : list) {
        registerService(false);
        m_pMainWindow->appendLogInfo(tr("zeroconf server detected: %1").arg(
            record.serviceName));
        m_pMainWindow->serverDetected(record.serviceName);
    }
}

void ZeroconfService::clientDetected(const QList<ZeroconfRecord>& list)
{
    for (const ZeroconfRecord& record : list) {
        m_pMainWindow->appendLogInfo(tr("zeroconf client detected: %1").arg(
            record.serviceName));
        m_pMainWindow->autoAddScreen(record.serviceName);
    }
}

void ZeroconfService::peerDetected(const QList<ZeroconfRecord>& list)
{
    for (const auto& record : list) {
        auto* resolver = new ZeroconfResolver(record, this);
        connect(resolver, &ZeroconfResolver::error,
                this, &ZeroconfService::errorHandle);
        connect(resolver, &ZeroconfResolver::resolved, this,
                [this](const ZeroconfRecord& resolved) {
            const auto protocol = resolved.txtRecords.value("proto");
            const auto nodeId = resolved.txtRecords.value("node");
            const auto capabilities = resolved.txtRecords.value("cap");
            const auto tls = resolved.txtRecords.value("tls");
            inputleap::PeerCapabilities parsedCapabilities = 0;
            if (protocol != "2" || nodeId.isEmpty() || tls != "mutual" ||
                !inputleap::peer_capabilities_from_string(
                    capabilities.toStdString(), parsedCapabilities) ||
                !inputleap::has_peer_capability(parsedCapabilities,
                    inputleap::PeerCapability::MutualTls) ||
                !inputleap::has_peer_capability(parsedCapabilities,
                    inputleap::PeerCapability::SessionGeneration)) {
                m_pMainWindow->appendLogError(
                    tr("ignored incompatible zeroconf peer: %1")
                        .arg(resolved.serviceName));
                return;
            }
            m_pMainWindow->appendLogInfo(
                tr("zeroconf peer detected: %1").arg(resolved.serviceName));
            m_pMainWindow->peerDetected(resolved.serviceName, resolved.hostName,
                                        resolved.port, nodeId, capabilities,
                                        resolved.txtRecords.value("fp"));
        });
        resolver->start();
    }
}

void ZeroconfService::errorHandle(DNSServiceErrorType errorCode)
{
    QMessageBox::critical(nullptr, tr("Zero configuration service"),
        tr("Error code: %1.").arg(errorCode));
}

bool ZeroconfService::registerService(bool server)
{
    bool result = true;

    if (!m_ServiceRegistered) {
        if (!m_zeroconfServer.listen()) {
            QMessageBox::critical(nullptr, tr("Zero configuration service"),
                tr("Unable to start the zeroconf: %1.")
                .arg(m_zeroconfServer.errorString()));
            result = false;
        }
        else {
            zeroconf_register_ = std::make_unique<ZeroconfRegister>(this);
            if (server) {
                zeroconf_register_->registerService(
                    ZeroconfRecord(tr("%1").arg(m_pMainWindow->getScreenName()),
                    QLatin1String(m_ServerServiceName), QString()),
                    m_zeroconfServer.serverPort());
            }
            else {
                zeroconf_register_->registerService(
                    ZeroconfRecord(tr("%1").arg(m_pMainWindow->getScreenName()),
                    QLatin1String(m_ClientServiceName), QString()),
                    m_zeroconfServer.serverPort());
            }

            m_ServiceRegistered = true;
        }
    }

    return result;
}

bool ZeroconfService::registerPeerService()
{
    QMap<QString, QString> attributes;
    attributes.insert("proto", "2");
    attributes.insert("node", m_pMainWindow->peerNodeId());
    attributes.insert("cap", QString::fromStdString(
        inputleap::peer_capabilities_to_string(inputleap::kDefaultPeerCapabilities)));
    attributes.insert("tls", "mutual");

    inputleap::FingerprintDatabase fingerprints;
    fingerprints.read(inputleap::DataDirectories::local_ssl_fingerprints_path());
    for (const auto& fingerprint : fingerprints.fingerprints()) {
        if (fingerprint.algorithm == "sha256") {
            attributes.insert("fp", QString::fromStdString(
                inputleap::FingerprintDatabase::to_db_line(fingerprint)));
            break;
        }
    }
    if (!attributes.contains("fp")) {
        m_pMainWindow->appendLogError(
            tr("peer discovery requires a local TLS certificate"));
        return false;
    }

    peer_register_ = std::make_unique<ZeroconfRegister>(this);
    connect(peer_register_.get(), &ZeroconfRegister::error,
            this, &ZeroconfService::errorHandle);
    peer_register_->registerService(
        ZeroconfRecord(m_pMainWindow->getScreenName(),
                       QLatin1String(m_PeerServiceName), QString(), QString(), 0,
                       attributes),
        m_pMainWindow->peerPort());
    return true;
}
