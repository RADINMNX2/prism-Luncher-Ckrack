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

#include "CrackedAccountDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include <utility>

namespace {

const QString s_dialogStyle = R"(
QDialog#CrackedAccountDialog {
    background-color: #1b1b22;
}
QLabel#title {
    color: #ffffff;
    font-size: 20px;
    font-weight: 700;
}
QLabel#subtitle {
    color: #8b8b9e;
    font-size: 13px;
}
QLabel#cardTitle {
    color: #e8e8f0;
    font-size: 14px;
    font-weight: 600;
}
QLabel#muted, QLabel#note {
    color: #8b8b9e;
    font-size: 12px;
}
QFrame#card {
    background-color: #23232c;
    border: 1px solid #32323e;
    border-radius: 14px;
}
QLabel#fieldLabel {
    color: #a9a9bc;
    font-size: 12px;
    font-weight: 600;
}
QLineEdit {
    background-color: #17171d;
    color: #ededf5;
    border: 1px solid #3a3a48;
    border-radius: 10px;
    padding: 9px 12px;
    font-size: 14px;
    selection-background-color: #6366f1;
    selection-color: #ffffff;
}
QLineEdit:focus {
    border: 1px solid #6366f1;
}
QCheckBox {
    color: #c8c8d6;
    spacing: 8px;
}
QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border-radius: 5px;
    border: 1px solid #3a3a48;
    background-color: #17171d;
}
QCheckBox::indicator:checked {
    background-color: #6366f1;
    border-color: #6366f1;
}
QPushButton#primary {
    background-color: #6366f1;
    color: #ffffff;
    border: none;
    border-radius: 10px;
    padding: 10px 24px;
    font-size: 14px;
    font-weight: 600;
}
QPushButton#primary:hover { background-color: #7c7ff5; }
QPushButton#primary:pressed { background-color: #5255d8; }
QPushButton#primary:disabled { background-color: #3a3a48; color: #77778a; }
QPushButton#ghost {
    background-color: transparent;
    color: #c0c0d0;
    border: 1px solid #3a3a48;
    border-radius: 10px;
    padding: 8px 16px;
    font-size: 13px;
}
QPushButton#ghost:hover { background-color: #2b2b36; }
QPushButton#ghost:disabled { color: #55556a; }
QPushButton#upload {
    background-color: #26262f;
    color: #d0d0e0;
    border: 1px dashed #4a4a5a;
    border-radius: 10px;
    padding: 8px 16px;
    font-size: 13px;
}
QPushButton#upload:hover { background-color: #31313d; border-color: #6366f1; }
QLabel#preview {
    background-color: #17171d;
    border: 1px solid #32323e;
    border-radius: 10px;
    color: #5c5c70;
}
QFrame#banner {
    background-color: rgba(99, 102, 241, 0.12);
    border: 1px solid rgba(99, 102, 241, 0.35);
    border-radius: 12px;
}
QLabel#bannerText {
    color: #b6b8ff;
    font-size: 12px;
}
)";

const QRegularExpression s_usernameRegExp("^[A-Za-z0-9_]{3,16}$");

QPixmap CrackedAccountDialog::imageFromData(const QByteArray& pngData)
{
    QPixmap pixmap;
    if (!pngData.isEmpty()) {
        pixmap.loadFromData(pngData, "PNG");
    }
    return pixmap;
}

bool CrackedAccountDialog::isValidPng(const QByteArray& pngData)
{
    static const QByteArray pngSignature = QByteArray::fromHex("89504e470d0a1a0a");
    return pngData.size() >= pngSignature.size() && pngData.left(pngSignature.size()) == pngSignature;
}

CrackedAccountDialog::CrackedAccountDialog(MinecraftAccountPtr account, QWidget* parent)
    : QDialog(parent), m_account(std::move(account))
{
    setObjectName("CrackedAccountDialog");
    setWindowTitle(tr("Cracked Account"));
    setMinimumWidth(480);

    setupUi();

    if (m_account) {
        // pre-fill existing values when editing
        m_nameEdit->setText(m_account->profileName());
        m_passwordEdit->setText(m_account->offlinePassword());
        m_slimCheck->setChecked(m_account->offlineSkinVariant() == "slim");
        m_skinPng = m_account->offlineSkinData();
        m_capePng = m_account->offlineCapeData();
    }

    updatePreviews();
    m_okButton->setEnabled(m_nameEdit->hasAcceptableInput());
}

MinecraftAccountPtr CrackedAccountDialog::createCrackedAccount(QWidget* parent)
{
    auto account = MinecraftAccount::createOffline(QString());
    CrackedAccountDialog dialog(account, parent);
    if (dialog.exec() == QDialog::Accepted) {
        return account;
    }
    return nullptr;
}

void CrackedAccountDialog::setupUi()
{
    setStyleSheet(s_dialogStyle);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto* title = new QLabel(tr("Cracked Account"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto* subtitle = new QLabel(tr("Full access to offline accounts — name, password, skin and cape."), this);
    subtitle->setObjectName("subtitle");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    // ---- Account card ----
    auto* accountCard = new QFrame(this);
    accountCard->setObjectName("card");
    auto* accountLayout = new QVBoxLayout(accountCard);
    accountLayout->setContentsMargins(18, 16, 18, 16);
    accountLayout->setSpacing(8);

    auto* accountTitle = new QLabel(tr("Account"), accountCard);
    accountTitle->setObjectName("cardTitle");
    accountLayout->addWidget(accountTitle);

    auto* nameLabel = new QLabel(tr("PROFILE NAME"), accountCard);
    nameLabel->setObjectName("fieldLabel");
    accountLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(accountCard);
    m_nameEdit->setPlaceholderText(tr("e.g. StevePlayer_2007"));
    m_nameEdit->setValidator(new QRegularExpressionValidator(s_usernameRegExp, this));
    accountLayout->addWidget(m_nameEdit);

    auto* passwordLabel = new QLabel(tr("PASSWORD"), accountCard);
    passwordLabel->setObjectName("fieldLabel");
    accountLayout->addWidget(passwordLabel);

    m_passwordEdit = new QLineEdit(accountCard);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Optional — stored locally only"));
    accountLayout->addWidget(m_passwordEdit);

    auto* passwordNote = new QLabel(tr("Cracked servers authenticate passwords in-game, so this is kept as a local record for your convenience."),
                                    accountCard);
    passwordNote->setObjectName("note");
    passwordNote->setWordWrap(true);
    accountLayout->addWidget(passwordNote);

    layout->addWidget(accountCard);

    // ---- Skin card ----
    auto* skinCard = new QFrame(this);
    skinCard->setObjectName("card");
    auto* skinLayout = new QVBoxLayout(skinCard);
    skinLayout->setContentsMargins(18, 16, 18, 16);
    skinLayout->setSpacing(10);

    auto* skinHeader = new QHBoxLayout();
    auto* skinTitle = new QLabel(tr("Skin"), skinCard);
    skinTitle->setObjectName("cardTitle");
    skinHeader->addWidget(skinTitle);
    skinHeader->addStretch();
    m_slimCheck = new QCheckBox(tr("Slim model (Alex)"), skinCard);
    skinHeader->addWidget(m_slimCheck);
    skinLayout->addLayout(skinHeader);

    auto* skinBody = new QHBoxLayout();
    skinBody->setSpacing(14);

    m_skinPreview = new QLabel(skinCard);
    m_skinPreview->setObjectName("preview");
    m_skinPreview->setFixedSize(96, 96);
    m_skinPreview->setAlignment(Qt::AlignCenter);
    skinBody->addWidget(m_skinPreview);

    auto* skinButtons = new QVBoxLayout();
    auto* uploadSkinBtn = new QPushButton(tr("Upload skin PNG..."), skinCard);
    uploadSkinBtn->setObjectName("upload");
    connect(uploadSkinBtn, &QPushButton::clicked, this, &CrackedAccountDialog::pickSkin);
    skinButtons->addWidget(uploadSkinBtn);

    auto* clearSkinBtn = new QPushButton(tr("Remove skin"), skinCard);
    clearSkinBtn->setObjectName("ghost");
    connect(clearSkinBtn, &QPushButton::clicked, this, &CrackedAccountDialog::clearSkin);
    skinButtons->addWidget(clearSkinBtn);
    skinButtons->addStretch();
    skinBody->addLayout(skinButtons);
    skinBody->addStretch();
    skinLayout->addLayout(skinBody);

    auto* skinNote = new QLabel(tr("Upload a standard 64x64 Minecraft skin PNG. All skin layers (hat, jacket, ...) are included in the PNG. "
                                   "Other players with CustomSkinLoader (or servers with a compatible skin plugin) will see it on cracked servers."),
                                skinCard);
    skinNote->setObjectName("note");
    skinNote->setWordWrap(true);
    skinLayout->addWidget(skinNote);

    layout->addWidget(skinCard);

    // ---- Cape card ----
    auto* capeCard = new QFrame(this);
    capeCard->setObjectName("card");
    auto* capeLayout = new QVBoxLayout(capeCard);
    capeLayout->setContentsMargins(18, 16, 18, 16);
    capeLayout->setSpacing(10);

    auto* capeTitle = new QLabel(tr("Cape"), capeCard);
    capeTitle->setObjectName("cardTitle");
    capeLayout->addWidget(capeTitle);

    auto* capeBody = new QHBoxLayout();
    capeBody->setSpacing(14);

    m_capePreview = new QLabel(capeCard);
    m_capePreview->setObjectName("preview");
    m_capePreview->setFixedSize(120, 56);
    m_capePreview->setAlignment(Qt::AlignCenter);
    capeBody->addWidget(m_capePreview);

    auto* capeButtons = new QVBoxLayout();
    auto* uploadCapeBtn = new QPushButton(tr("Upload cape PNG..."), capeCard);
    uploadCapeBtn->setObjectName("upload");
    connect(uploadCapeBtn, &QPushButton::clicked, this, &CrackedAccountDialog::pickCape);
    capeButtons->addWidget(uploadCapeBtn);

    auto* clearCapeBtn = new QPushButton(tr("Remove cape"), capeCard);
    clearCapeBtn->setObjectName("ghost");
    connect(clearCapeBtn, &QPushButton::clicked, this, &CrackedAccountDialog::clearCape);
    capeButtons->addWidget(clearCapeBtn);
    capeButtons->addStretch();
    capeBody->addLayout(capeButtons);
    capeBody->addStretch();
    capeLayout->addLayout(capeBody);

    layout->addWidget(capeCard);

    // ---- CustomSkinLoader banner ----
    auto* banner = new QFrame(this);
    banner->setObjectName("banner");
    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(16, 12, 16, 12);
    auto* bannerText = new QLabel(banner);
    bannerText->setObjectName("bannerText");
    bannerText->setWordWrap(true);
    bannerText->setText(tr("In-game: the launcher writes your skin & cape into each instance's CustomSkinLoader folder "
                           "(LocalSkin) on launch. Install the CustomSkinLoader mod in an instance to see them, and other "
                           "CustomSkinLoader users will see them too."));
    bannerLayout->addWidget(bannerText);
    layout->addWidget(banner);

    // ---- Buttons ----
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setObjectName("primary");
    m_okButton->setText(tr("Save"));
    buttonBox->button(QDialogButtonBox::Cancel)->setObjectName("ghost");
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CrackedAccountDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CrackedAccountDialog::reject);
    layout->addWidget(buttonBox);

    connect(m_nameEdit, &QLineEdit::textEdited, this, &CrackedAccountDialog::onNameEdited);
}

QPixmap CrackedAccountDialog::faceFromSkin(const QByteArray& pngData, int size) const
{
    auto texture = imageFromData(pngData);
    if (texture.isNull()) {
        return QPixmap();
    }
    QPixmap face(8, 8);
    face.fill(Qt::transparent);
    QPainter painter(&face);
    painter.drawPixmap(0, 0, texture.copy(8, 8, 8, 8));
    painter.drawPixmap(0, 0, texture.copy(40, 8, 8, 8));
    return face.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void CrackedAccountDialog::updatePreviews()
{
    if (m_skinPreview) {
        auto face = faceFromSkin(m_skinPng, 80);
        if (face.isNull()) {
            m_skinPreview->setText(tr("No skin"));
        } else {
            m_skinPreview->setPixmap(face);
        }
    }
    if (m_capePreview) {
        auto cape = imageFromData(m_capePng);
        if (cape.isNull()) {
            m_capePreview->setText(tr("No cape"));
        } else {
            m_capePreview->setPixmap(cape.scaled(112, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void CrackedAccountDialog::pickSkin()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Select skin"), QString(), tr("PNG images (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto data = file.readAll();
    if (!isValidPng(data)) {
        QMessageBox::warning(this, tr("Invalid file"), tr("The selected file is not a valid PNG image."));
        return;
    }
    m_skinPng = data;
    updatePreviews();
}

void CrackedAccountDialog::clearSkin()
{
    m_skinPng = QByteArray();
    updatePreviews();
}

void CrackedAccountDialog::pickCape()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Select cape"), QString(), tr("PNG images (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto data = file.readAll();
    if (!isValidPng(data)) {
        QMessageBox::warning(this, tr("Invalid file"), tr("The selected file is not a valid PNG image."));
        return;
    }
    m_capePng = data;
    updatePreviews();
}

void CrackedAccountDialog::clearCape()
{
    m_capePng = QByteArray();
    updatePreviews();
}

void CrackedAccountDialog::onNameEdited(const QString& text)
{
    Q_UNUSED(text)
    m_okButton->setEnabled(m_nameEdit->hasAcceptableInput());
}

void CrackedAccountDialog::accept()
{
    if (!m_account) {
        reject();
        return;
    }
    if (!m_nameEdit->hasAcceptableInput()) {
        return;
    }
    m_account->setOfflineName(m_nameEdit->text().trimmed());
    m_account->setOfflinePassword(m_passwordEdit->text());
    m_account->setOfflineSkinData(m_skinPng, m_slimCheck->isChecked() ? "slim" : "classic");
    m_account->setOfflineCapeData(m_capePng);
    QDialog::accept();
}
