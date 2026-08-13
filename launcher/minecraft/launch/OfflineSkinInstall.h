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

#pragma once

#include <launch/LaunchStep.h>
#include <minecraft/auth/AuthSession.h>

class MinecraftInstance;

/*!
 * Installs the custom skin/cape of an offline (cracked) account into the
 * instance for CustomSkinLoader's "LocalSkin" loader and for OptiFine.
 *
 * Files written into the game directory:
 *   config/customskinloader/ModSkins/<username>.png
 *   config/customskinloader/ModSkins/<username>_cape.png
 *   config/customskinloader/ModSkins/<username>.json   (slim model, optional)
 *   config/customskinloader/CustomSkinLoader.json      (merged LocalSkin loadlist)
 *   skins/<username>.png                               (OptiFine local skin)
 *   capes/<username>.png                               (OptiFine local cape)
 *
 * This is a no-op for Microsoft accounts or offline accounts without a skin/cape.
 */
class OfflineSkinInstall : public LaunchStep {
    Q_OBJECT
   public:
    explicit OfflineSkinInstall(LaunchTask* parent, AuthSessionPtr session);
    ~OfflineSkinInstall() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

   private:
    //! Merges our LocalSkin loadlist entry into config/customskinloader/CustomSkinLoader.json,
    //! preserving any existing user configuration.
    void writeMergedConfig(const QString& cslDir);

    //! Writes the skin/cape for CustomSkinLoader's LocalSkin loader. Returns true if anything was written.
    bool installCustomSkinLoader(MinecraftInstance* instance, const QString& username, const QByteArray& skinData, const QByteArray& capeData, const QString& variant);

    //! Writes the skin/cape for OptiFine (skins/ and capes/ in the game dir). Returns true if anything was written.
    bool installOptiFineFiles(MinecraftInstance* instance, const QString& username, const QByteArray& skinData, const QByteArray& capeData);

    AuthSessionPtr m_session;
};