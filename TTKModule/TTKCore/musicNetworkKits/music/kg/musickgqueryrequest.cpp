#include "musickgqueryrequest.h"

MusicKGQueryRequest::MusicKGQueryRequest(QObject *parent)
    : MusicAbstractQueryRequest(parent)
{
    m_queryServer = QUERY_KG_INTERFACE;
    m_pageSize = 30;
}

void MusicKGQueryRequest::startToSearch(QueryType type, const QString &text)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(text));
    m_currentType = type;
    m_searchText = text.trimmed();

    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();

    startToPage(0);
}

void MusicKGQueryRequest::startToPage(int offset)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToPage %2").arg(getClassName()).arg(offset));
    deleteAll();

    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KG_SONG_SEARCH_URL, false).arg(m_searchText).arg(offset + 1).arg(m_pageSize);
    m_interrupt = true;
    m_pageTotal = 0;
    m_pageIndex = offset;

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KG_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    m_reply = m_manager->get(request);
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicKGQueryRequest::startToSingleSearch(const QString &text)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSingleSearch %2").arg(getClassName()).arg(text));

    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KG_SONG_INFO_URL, false).arg(text);
    m_interrupt = true;

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KG_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), SLOT(singleDownLoadFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicKGQueryRequest::downLoadFinished()
{
    if(!m_reply || !m_manager)
    {
        deleteAll();
        return;
    }

    TTK_LOGGER_INFO(QString("%1 downLoadFinished").arg(getClassName()));
    m_interrupt = false;

    if(m_reply->error() == QNetworkReply::NoError)
    {
        const QByteArray &bytes = m_reply->readAll();

        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("data"))
            {
                value = value["data"].toMap();
                m_pageTotal = value["total"].toInt();
                const QVariantList &datas = value["info"].toList();
                for(const QVariant &var : qAsConst(datas))
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    MusicObject::MusicSongInformation musicInfo;
                    musicInfo.m_singerName = MusicUtils::String::illegalCharactersReplaced(value["singername"].toString());
                    musicInfo.m_songName = MusicUtils::String::illegalCharactersReplaced(value["songname"].toString());
                    musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt() * 1000);

                    musicInfo.m_songId = value["hash"].toString();
                    musicInfo.m_albumId = value["album_id"].toString();
                    musicInfo.m_albumName = MusicUtils::String::illegalCharactersReplaced(value["album_name"].toString());

                    musicInfo.m_year = QString();
                    musicInfo.m_discNumber = "1";
                    musicInfo.m_trackNumber = "0";

                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                    readFromMusicSongLrcAndPicture(&musicInfo);
                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;

                    if(!m_querySimplify)
                    {
                        if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                        readFromMusicSongAttribute(&musicInfo, value, m_searchQuality, m_queryAllRecords);
                        if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;

                        if(musicInfo.m_songAttrs.isEmpty())
                        {
                            continue;
                        }

                        MusicSearchedItem item;
                        item.m_songName = musicInfo.m_songName;
                        item.m_singerName = musicInfo.m_singerName;
                        item.m_albumName = musicInfo.m_albumName;
                        item.m_time = musicInfo.m_timeLength;
                        item.m_type = mapQueryServerString();
                        Q_EMIT createSearchedItem(item);
                    }
                    m_musicSongInfos << musicInfo;
                }
            }
        }
    }

    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}

void MusicKGQueryRequest::singleDownLoadFinished()
{
    QNetworkReply *reply = TTKObject_cast(QNetworkReply*, QObject::sender());

    TTK_LOGGER_INFO(QString("%1 singleDownLoadFinished").arg(getClassName()));
    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();
    m_interrupt = false;

    if(reply && m_manager &&reply->error() == QNetworkReply::NoError)
    {
        const QByteArray &bytes = reply->readAll();

        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value["errcode"].toInt() == 0 && value.contains("data"))
            {
                value = value["data"].toMap();
                MusicObject::MusicSongInformation musicInfo;
                musicInfo.m_songId = value["hash"].toString();
                musicInfo.m_singerName = MusicUtils::String::illegalCharactersReplaced(value["singername"].toString());
                musicInfo.m_songName = MusicUtils::String::illegalCharactersReplaced(value["songname"].toString());
                musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt() * 1000);
                musicInfo.m_artistId = QString::number(value["singerid"].toULongLong());
                musicInfo.m_smallPicUrl = value["imgurl"].toString().replace("{size}", "480");
                musicInfo.m_lrcUrl = MusicUtils::Algorithm::mdII(KG_SONG_LRC_URL, false)
                                                        .arg(musicInfo.m_songName).arg(musicInfo.m_songId)
                                                        .arg(value["duration"].toInt() * 1000);
                const QVariantList &albumArray = value["album"].toList();
                for(const QVariant &albumValue : qAsConst(albumArray))
                {
                    if(albumValue.isNull())
                    {
                        continue;
                    }
                    const QVariantMap &albumMap = albumValue.toMap();
                    musicInfo.m_albumId = albumMap["album_audio_id"].toString();
                    musicInfo.m_albumName = MusicUtils::String::illegalCharactersReplaced(albumMap["album_name"].toString());
                }

                musicInfo.m_year = QString();
                musicInfo.m_discNumber = "1";
                musicInfo.m_trackNumber = "0";

                if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                readFromMusicSongAttribute(&musicInfo, value["extra"].toMap(), m_searchQuality, true);
                if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;

                if(!musicInfo.m_songAttrs.isEmpty())
                {
                    MusicSearchedItem item;
                    item.m_songName = musicInfo.m_songName;
                    item.m_singerName = musicInfo.m_singerName;
                    item.m_albumName = musicInfo.m_albumName;
                    item.m_time = musicInfo.m_timeLength;
                    item.m_type = mapQueryServerString();
                    Q_EMIT createSearchedItem(item);

                    m_musicSongInfos << musicInfo;
                }
            }
        }
    }

    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}
