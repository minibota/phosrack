// SPDX-License-Identifier: GPL-3.0-or-later
#include "protocol.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
namespace Phosrack {
QByteArray pkceChallenge(const QByteArray &verifier) {
    return QCryptographicHash::hash(verifier, QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}
QByteArray randomToken() {
    QByteArray bytes(32, Qt::Uninitialized);
    for (int i=0;i<32;i+=4) {
        const auto word=QRandomGenerator::system()->generate();
        for(int j=0;j<4;++j) bytes[i+j]=char((word >> (j*8)) & 255);
    }
    return bytes.toHex();
}
Callback parseCallback(const QByteArray &request, const QString &expectedState) {
    Callback result;
    if(expectedState.isEmpty() || request.size()>8192) return result;
    const auto parts=request.left(request.indexOf('\r')).split(' ');
    if(parts.size()!=3 || parts[0]!="GET" || !parts[1].startsWith("/callback?")) return result;
    const QUrl url=QUrl::fromEncoded("http://127.0.0.1"+parts[1]);
    const QUrlQuery query(url);
    int stateCount=0,codeCount=0,errorCount=0;
    for(const auto &item:query.queryItems()) {
        if(item.first=="state") ++stateCount;
        if(item.first=="code") ++codeCount;
        if(item.first=="error") ++errorCount;
    }
    if(url.path()!="/callback" || stateCount!=1 || query.queryItemValue("state")!=expectedState
        || codeCount+errorCount!=1) return result;
    result.code=query.queryItemValue("code",QUrl::FullyDecoded);
    result.error=query.queryItemValue("error",QUrl::FullyDecoded);
    result.valid=!result.code.isEmpty() || !result.error.isEmpty();
    return result;
}
QVariantMap playbackState(const QJsonObject &o) {
    const auto item=o.value("item").toObject();
    const auto device=o.value("device").toObject();
    const auto album=item.value("album").toObject();
    const auto blocked=o.value("actions").toObject().value("disallows").toObject();
    QStringList artists;
    for(const auto &artist:item.value("artists").toArray()) artists<<artist.toObject().value("name").toString();
    if(artists.isEmpty()) artists<<item.value("show").toObject().value("name").toString();
    auto images=album.value("images").toArray();
    if(images.isEmpty()) images=item.value("images").toArray();
    const bool control=!device.isEmpty() && !device.value("is_restricted").toBool();
    const bool playing=o.value("is_playing").toBool();
    const int duration=item.value("duration_ms").toInt();
    return {{"title",item.value("name").toString()}, {"artist",artists.join(", ")},
        {"album",album.value("name").toString()}, {"art",images.isEmpty()?QString():images.first().toObject().value("url").toString()},
        {"url",item.value("external_urls").toObject().value("spotify").toString()},
        {"duration",duration}, {"position",std::clamp(o.value("progress_ms").toInt(),0,std::max(0,duration))},
        {"playing",playing},{"shuffle",o.value("shuffle_state").toBool()},
        {"repeat",o.value("repeat_state").toString("off")}, {"device",device.value("name").toString()},
        {"deviceId",device.value("id").toString()}, {"volume",device.value("volume_percent").toInt()},
        {"canControl",control}, {"canToggle",control&&!blocked.value(playing?"pausing":"resuming").toBool()},
        {"canNext",control&&!blocked.value("skipping_next").toBool()},
        {"canPrevious",control&&!blocked.value("skipping_prev").toBool()},
        {"canSeek",control&&duration>0&&!blocked.value("seeking").toBool()},
        {"canVolume",control&&device.value("supports_volume").toBool(!device.value("volume_percent").isNull())},
        {"canShuffle",control&&!blocked.value("toggling_shuffle").toBool()},
        {"canRepeat",control&&!blocked.value("toggling_repeat_context").toBool()&&!blocked.value("toggling_repeat_track").toBool()}};
}
}
