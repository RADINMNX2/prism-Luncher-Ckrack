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

#include <QDialog>

#include "minecraft/auth/MinecraftAccount.h"

class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QFrame;

/*!
 * A premium, dark-styled dialog for creating and editing offline (cracked) accounts.
 *
 * It supports:
 *  - editing the account name and a locally stored password,
 *  - uploading a custom skin PNG (including skin layers, slim/classic model),
 *  - uploading a custom cape PNG.
 *
 * The uploaded skin/cape is applied in-game through CustomSkinLoader's LocalSkin
 * loader when the instance is launched (see minecraft/launch/OfflineSkinInstall).
 */
class CrackedAccountDialog final : public QDialog {
    Q_OBJECT
   public:
    /*!
     * Opens the dialog for an existing (or freshly created) offline account.
     * The account is only modified when the dialog is accepted.
     */
    explicit CrackedAccountDialog(MinecraftAccountPtr account, QWidget* parent = nullptr);
    ~CrackedAccountDialog() override = default;

    /*!
     * Creates a new offline (cracked) account via the dialog.
     * Returns nullptr if the user cancelled.
     */
    static MinecraftAccountPtr createCrackedAccount(QWidget* parent);

   private slots:
    void pickSkin();
    void clearSkin();
    void pickCape();
    void clearCape();
    void onNameEdited(const QString& text);

   private:
    void setupUi();
    void updatePreviews();
    QPixmap faceFromSkin(const QByteArray& pngData, int size) const;
    static QPixmap imageFromData(const QByteArray& pngData);
    static bool isValidPng(const QByteArray& pngData);
    void accept() override;

    MinecraftAccountPtr m_account;

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QCheckBox* m_slimCheck = nullptr;
    QLabel* m_skinPreview = nullptr;
    QLabel* m_capePreview = nullptr;
    QPushButton* m_okButton = nullptr;

    QByteArray m_skinPng;
    QByteArray m_capePng;
};
