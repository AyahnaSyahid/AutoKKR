// AboutKKRDialog.cpp
#include "aboutkkrdialog.h"
#include "polledsettings.h"
#include "tokenrequestdialog.h"
#include "tokenrefilldialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QAction>
#include <QFont>

namespace AboutBuild {
    std::string_view getTimestamp();
    std::string_view getGitHash();
}

AboutKKRDialog::AboutKKRDialog(PolledSettings *ps, QWidget *parent) : m_ps(ps), QDialog(parent) {
    QString t("(Token Info) Digunakan : %1, Sisa : %2\nM ID: %3");
    auto adata = ps->appData();
    auto infoLabel = new QLabel(t.arg(adata.UsageCounter, 6, 10, QChar(' '))
                                 .arg(adata.CounterLeft, 6, 10, QChar(' '))
                                 .arg(adata.MachineID), this);
    auto ifont = infoLabel->font();
    ifont.setBold(true);
    infoLabel->setFont(ifont);
    infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setWindowTitle("Tentang Auto KK");
    setFixedSize(600, 300); // Ukuran dialog, bisa disesuaikan

    QString indoText = R"(
<h2>Tentang Auto KK</h2>
<p>Auto KK adalah aplikasi untuk automatisasi pelayoutan desain kertas kado. Dirancang untuk memudahkan pengguna dalam mengatur layout desain secara otomatis, sehingga proses pembuatan kertas kado lebih efisien dan presisi.</p>
<p>Dikembangkan menggunakan:</p>
<ul>
    <li>Qt 6.8.3 : Framework UI cross-platform. Lisensi: GNU Lesser General Public License (LGPL) untuk edisi open-source. Lihat detail: <a href="https://www.qt.io/licensing/">https://www.qt.io/licensing/</a></li>
    <li>OpenCV 4.x: Library untuk pengolahan gambar dan visi komputer. Lisensi: Apache License 2.0. Lihat detail: <a href="https://opencv.org/license/">https://opencv.org/license/</a></li>
    <li>LittleCMS: Engine manajemen warna. Lisensi: MIT License. Lihat detail: <a href="https://littlecms.com/color-engine/">https://littlecms.com/color-engine/</a></li>
</ul>
<p>
  Versi: 1.2<br>Dibuat oleh: Noer Holis K<br>Hak Cipta © 2025 Noer C. Semua hak dilindungi.
  <br> Build Time : %1 [%2] </br>
</p>
<p>Atas permitaan dari : Balad</p>
<p>Terima kasih telah menggunakan Auto KK! Jika ada saran atau bug, hubungi<br>Email: ayah.syahid@gmail.com</p>
    )";
    
    QString englishText = R"(
<h2>About Auto KK</h2>
<p>Auto KK is an application for automating the layout design of gift wrapping paper. Designed to make it easier for users to arrange design layouts automatically, making the gift wrapping creation process more efficient and precise.</p>
<p>Developed using:</p>
<ul>
    <li>Qt 6.8.3 : Cross-platform UI framework. License: GNU Lesser General Public License (LGPL) for open-source edition. See details: <a href="https://www.qt.io/licensing/">https://www.qt.io/licensing/</a></li>
    <li>OpenCV 4.x: Library for image processing and computer vision. License: Apache License 2.0. See details: <a href="https://opencv.org/license/">https://opencv.org/license/</a></li>
    <li>LittleCMS: Color management engine. License: MIT License. See details: <a href="https://littlecms.com/color-engine/">https://littlecms.com/color-engine/</a></li>
</ul>
<p>
  Version: 1.0<br>Created by: Noer Holis K<br>Copyright © 2025 Noer C. All rights reserved.
  <br> Build Time : %1 [%2] </br>
</p>
<p>Thank you for using Auto KK! If you have suggestions or bugs, contact<br>Email: ayah.syahid@gmail.com</p>
    )";
    QTabWidget *tabWidget = new QTabWidget(this);

    // Tab Bahasa Indonesia
    QWidget *indonesianTab = new QWidget();
    QVBoxLayout *indonesianLayout = new QVBoxLayout(indonesianTab);
    QLabel *indonesianLabel = new QLabel(indoText.arg(AboutBuild::getTimestamp().data(), AboutBuild::getGitHash().data()));
    
    indonesianLabel->setWordWrap(true);
    indonesianLabel->setOpenExternalLinks(true); // Membuat link clickable
    indonesianLayout->addWidget(indonesianLabel);
    auto *infoLayout = new QHBoxLayout();
    auto *btnLayout = new QVBoxLayout();
    QPushButton *reqToken = new QPushButton("Token Requests", this);
    QPushButton *isiToken = new QPushButton("Isi Token", this);
    infoLayout->addWidget(infoLabel, 1);
    btnLayout->addWidget(reqToken, 0);
    btnLayout->addWidget(isiToken, 0);
    infoLayout->addLayout(btnLayout, 0);
    indonesianLayout->addLayout(infoLayout);
    tabWidget->addTab(indonesianTab, "Bahasa Indonesia");

    // Tab Bahasa Inggris
    QWidget *englishTab = new QWidget();
    QVBoxLayout *englishLayout = new QVBoxLayout(englishTab);
    QLabel *englishLabel = new QLabel(englishText.arg(AboutBuild::getTimestamp().data(), AboutBuild::getGitHash().data()));
    englishLabel->setWordWrap(true);
    englishLabel->setOpenExternalLinks(true); // Membuat link clickable
    englishLayout->addWidget(englishLabel);
    tabWidget->addTab(englishTab, "English");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
    trusts = new QAction("OpenTrusts", this);
    trusts->setShortcuts(QKeySequence::Cut);
    connect(trusts, &QAction::triggered, this, &AboutKKRDialog::displayTrusts);
    addAction(trusts);
    connect(reqToken, &QPushButton::clicked, this, &AboutKKRDialog::requestToken);
    connect(isiToken, &QPushButton::clicked, this, &AboutKKRDialog::refillToken);
}

void AboutKKRDialog::displayTrusts() {
  QDialog dl;
  QVBoxLayout ly;
  QLabel lab(QString("Trust Level: %1").arg(QString::number(m_ps->appData().trustLevel, 'f', 3)), &dl);
  lab.setAlignment(Qt::AlignCenter);
  dl.setMinimumSize(200, 100);
  ly.addWidget(&lab);
  dl.setLayout(&ly);
  dl.exec();
}

void AboutKKRDialog::refillToken() {
  TokenRefillDialog *trd = new TokenRefillDialog(m_ps, this);
  trd->setAttribute(Qt::WA_DeleteOnClose);
  connect(trd, &QDialog::accepted, [=]() {
    QMessageBox::information(this, "Berhasil", "Anda mungkin harus menutup dialog terlebih dahulu untuk melihat Counter Token");});
  trd->open();
}

void AboutKKRDialog::requestToken() {
  TokenRequestDialog *trd = new TokenRequestDialog(m_ps, this);
  trd->setAttribute(Qt::WA_DeleteOnClose);
  trd->open();
}

AboutKKRDialog::~AboutKKRDialog() {
    // Tidak perlu cleanup manual karena Qt menangani children
}