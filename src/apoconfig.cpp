// SPDX-License-Identifier: GPL-3.0-or-later
#include "apoconfig.h"
#include <QRegularExpression>
#include <algorithm>
#include <array>
#include <cmath>
namespace {
double bounded(double x) { return std::isfinite(x) ? std::clamp(x,-12.0,12.0) : 0.0; }
}
namespace Phosrack {
QString apoConfiguration(const QVariantList &gains, double preamp, bool enabled) {
    QString result="# Phosrack - managed local Equalizer APO filters\n";
    result+="Preamp: "+QString::number(enabled?bounded(preamp):0.0,'f',3)+" dB\n";
    if(!enabled)return result;
    constexpr std::array<double,10> freq{31.25,62.5,125,250,500,1000,2000,4000,8000,16000};
    for(int i=0;i<10;++i)result+=QString("Filter %1: ON PK Fc %2 Hz Gain %3 dB Q 1.414213562\n")
        .arg(i+1).arg(QString::number(freq[i],'g',8)).arg(QString::number(bounded(gains.value(i).toDouble()),'f',3));
    return result;
}
QString attachApoConfiguration(const QString &existing) {
    static const QRegularExpression activeInclude("^\\s*Include:\\s*phosrack[.]txt\\s*$",
        QRegularExpression::MultilineOption|QRegularExpression::CaseInsensitiveOption);
    if(activeInclude.match(existing).hasMatch())return existing;
    QString result=existing;
    if(!result.isEmpty()&&!result.endsWith('\n'))result+='\n';
    result+="\n# BEGIN PHOSRACK - optional local equalizer\nDevice: all\nChannel: all\nInclude: phosrack.txt\n# END PHOSRACK\n";
    return result;
}
ApoAttachment attachApoConfigurationBytes(const QByteArray &existing) {
    // APO files may contain legacy ANSI or a UTF-8 BOM. Preserve every original
    // byte instead of decoding/re-encoding the user's unrelated filter chain.
    // UTF-16/32 (or binary data) cannot safely accept an appended ASCII include.
    if(existing.startsWith(QByteArray::fromHex("fffe"))||existing.startsWith(QByteArray::fromHex("feff"))||existing.contains('\0'))
        return {existing,false};
    QByteArray searchable=existing;
    if(searchable.startsWith(QByteArray::fromHex("efbbbf")))searchable.remove(0,3);
    static const QRegularExpression activeInclude("^\\s*Include:\\s*phosrack[.]txt\\s*$",
        QRegularExpression::MultilineOption|QRegularExpression::CaseInsensitiveOption);
    if(activeInclude.match(QString::fromLatin1(searchable)).hasMatch())return {existing,true};
    QByteArray result=existing;
    if(!result.isEmpty()&&!result.endsWith('\n'))result+='\n';
    result+="\n# BEGIN PHOSRACK - optional local equalizer\nDevice: all\nChannel: all\nInclude: phosrack.txt\n# END PHOSRACK\n";
    return {result,true};
}
}
