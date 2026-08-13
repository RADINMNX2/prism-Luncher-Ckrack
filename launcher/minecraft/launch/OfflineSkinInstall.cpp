// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "OfflineSkinInstall.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "FileSystem.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/auth/AccountList.h"

OfflineSkinInstall::OfflineSkinInstall(LaunchTask* parent, AuthSessionPtr session)
    : LaunchStep(parent), m_session(std::move(session))
{}

void OfflineSkinInstall::executeTask()
{
    if (!m_session || m_session->user_type != QLatin1String("offline")) {
        emitSucceeded();
        return;
    }

    auto instance = m_parent->instance();
    auto account = APPLICATION->accounts()->getAccountByProfileName(m_session->player_name);
    if (!account || !account->isOffline()) {
        emitSucceeded();
        return;
    }

    const auto skinData = account->offlineSkinData();
    const auto capeData = account->offlineCapeData();
    if (skinData.isEmpty() && capeData.isEmpty()) {
        emitSucceeded();
        return;
    }

    const QString username = m_session->player_name;
    const QString cslDir = FS::PathCombine(instance->gameRoot(), "config", "customskinloader");
    const QString modSkinsDir = FS::PathCombine(cslDir, "ModSkins");

    if (!FS::ensureFolderPathExists(modSkinsDir)) {
        emit logLine(tr("Failed to create the CustomSkinLoader folder; skin and cape won't be applied."), MessageLevel::Error);
        emitFailed(tr("Couldn't create the CustomSkinLoader folder."));
        return;
    }

    try {
        if (!skinData.isEmpty()) {
            FS::write(FS::PathCombine(modSkinsDir, username + ".png"), skinData);
            if (account->offlineSkinVariant() == "slim") {
                FS::write(FS::PathCombine(modSkinsDir, username + ".json"), QByteArrayLiteral("{\"model\":\"slim\"}"));
            }
        }
        if (!capeData.isEmpty()) {
            FS::write(FS::PathCombine(modSkinsDir, username + "_cape.png"), capeData);
        }

        writeMergedConfig(cslDir);
    } catch (const FileSystemException& e) {
        emit logLine(tr("Couldn't install the cracked skin/cape: %1").arg(e.cause()), MessageLevel::Error);
        emitFailed(tr("Couldn't install the cracked skin/cape."));
        return;
    }

    emit logLine(tr("Installed cracked skin and cape for '%1' (CustomSkinLoader).").arg(username), MessageLevel::Launcher);
    emitSucceeded();
}

void OfflineSkinInstall::writeMergedConfig(const QString& cslDir)
{
    const QString configPath = FS::PathCombine(cslDir, "CustomSkinLoader.json");
    QJsonObject root;
    QJsonArray loadlist;

    // Preserve the user's existing CustomSkinLoader configuration, if any.
    try {
        const auto existing = FS::read(configPath);
        if (!existing.isEmpty()) {
            QJsonParseError err;
            const auto doc = QJsonDocument::fromJson(existing, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                root = doc.object();
                loadlist = root.value("loadlist").toArray();
            }
        }
    } catch (const FileSystemException&) {
        // no existing config, start fresh
    }

    // Add our LocalSkin loader only if it isn't already present.
    bool hasLocalSkin = false;
    for (const auto& entryV : loadlist) {
        if (entryV.isObject() && entryV.toObject().value("type").toString() == QLatin1String("LocalSkin")) {
            hasLocalSkin = true;
            break;
        }
    }
    if (!hasLocalSkin) {
        QJsonObject localSkin;
        localSkin["name"] = "PrismLauncher Local Skin";
        localSkin["type"] = "LocalSkin";
        localSkin["root"] = "./config/customskinloader/ModSkins/";
        localSkin["checkPNG"] = true;
        localSkin["enable"] = true;
        loadlist.append(localSkin);
    }
    root["loadlist"] = loadlist;

    // Sensible defaults for keys that CustomSkinLoader may expect.
    if (!root.contains("enableLocalSkin")) {
        root["enableLocalSkin"] = true;
    }
    if (!root.contains("localSkinConfig")) {
        QJsonObject localSkinConfig;
        localSkinConfig["enableAlexSkin"] = true;
        localSkinConfig["enableCape"] = true;
        root["localSkinConfig"] = localSkinConfig;
    }
    if (!root.contains("enableElytraByCape")) {
        root["enableElytraByCape"] = true;
    }
    if (!root.contains("forceLoadSkins")) {
        root["forceLoadSkins"] = true;
    }

    FS::write(configPath, QJsonDocument(root).toJson(QJsonDocument::Indented));
}
