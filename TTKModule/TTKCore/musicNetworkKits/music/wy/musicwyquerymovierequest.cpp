#include "musicwyquerymovierequest.h"
#include "musicsemaphoreloop.h"
#include "musiccoreutils.h"

MusicWYQueryMovieRequest::MusicWYQueryMovieRequest(QObject *parent)
    : MusicQueryMovieRequest(parent)
{
    m_queryServer = QUERY_WY_INTERFACE;
    m_pageSize = 40;
}

void MusicWYQueryMovieRequest::startToSearch(QueryType type, const QString &text)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(text));
    deleteAll();

    m_searchText = text.trimmed();
    m_currentType = type;
    m_interrupt = true;

    QNetworkRequest request;
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    const QByteArray &parameter = makeTokenQueryUrl(&request,
                      MusicUtils::Algorithm::mdII(WY_SONG_SEARCH_URL, false),
                      MusicUtils::Algorithm::mdII(WY_SONG_SEARCH_DATA_URL, false).arg(m_searchText).arg(m_pageSize).arg(0).toUtf8());
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    MusicObject::setSslConfiguration(&request);

    m_reply = m_manager->post(request, parameter);
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicWYQueryMovieRequest::startToPage(int offset)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(offset));
    deleteAll();

    m_pageTotal = 0;
    m_pageSize = 20;
    m_interrupt = true;

    QNetworkRequest request;
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    const QByteArray &parameter = makeTokenQueryUrl(&request,
                      MusicUtils::Algorithm::mdII(WY_ARTIST_MOVIE_URL, false),
                      MusicUtils::Algorithm::mdII(WY_ARTIST_MOVIE_DATA_URL, false).arg(m_searchText).arg(m_pageSize*offset).arg(m_pageSize));
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    MusicObject::setSslConfiguration(&request);

    m_reply = m_manager->post(request, parameter);
    connect(m_reply, SIGNAL(finished()), SLOT(pageDownLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicWYQueryMovieRequest::startToSingleSearch(const QString &text)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSingleSearch %2").arg(getClassName()).arg(text));

    m_searchText = text.trimmed();
    m_interrupt = true;

    QTimer::singleShot(MT_MS, this, SLOT(singleDownLoadFinished()));
}

void MusicWYQueryMovieRequest::downLoadFinished()
{
    if(!m_reply || !m_manager)
    {
        deleteAll();
        return;
    }

    TTK_LOGGER_INFO(QString("%1 downLoadFinished").arg(getClassName()));
    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();
    m_interrupt = false;

    if(m_reply->error() == QNetworkReply::NoError)
    {
        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(m_reply->readAll(), &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("code") && value["code"].toInt() == 200)
            {
                value = value["result"].toMap();
                const QVariantList &datas = value["songs"].toList();
                for(const QVariant &var : qAsConst(datas))
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    const qint64 mvid = value["mv"].toLongLong();
                    if(mvid == 0)
                    {
                        continue;
                    }

                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                    startMVListQuery(mvid);
                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                }
            }
        }
    }

    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}

void MusicWYQueryMovieRequest::pageDownLoadFinished()
{
    if(!m_reply || !m_manager)
    {
        deleteAll();
        return;
    }

    TTK_LOGGER_INFO(QString("%1 pageDownLoadFinished").arg(getClassName()));
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
            if(value["code"].toInt() == 200 && value.contains("mvs"))
            {
                if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                getArtistMvsCount(m_searchText.toLongLong());
                if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;

                const QVariantList &datas = value["mvs"].toList();
                for(const QVariant &var : qAsConst(datas))
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();

                    if(m_interrupt) return;

                    MusicResultsItem info;
                    info.m_id = QString::number(value["id"].toLongLong());
                    info.m_coverUrl = value["imgurl"].toString();
                    info.m_name = value["name"].toString();
                    info.m_updateTime.clear();
                    Q_EMIT createMovieInfoItem(info);
                }
            }
        }
    }

    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}

void MusicWYQueryMovieRequest::singleDownLoadFinished()
{
    TTK_LOGGER_INFO(QString("%1 singleDownLoadFinished").arg(getClassName()));

    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();
    m_interrupt = false;

    const qint64 mvid = m_searchText.toLongLong();
    if(mvid != 0)
    {
        if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
        startMVListQuery(mvid);
        if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    }

    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}

void MusicWYQueryMovieRequest::startMVListQuery(qint64 id)
{
    QNetworkRequest request;
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    const QByteArray &parameter = makeTokenQueryUrl(&request,
                      MusicUtils::Algorithm::mdII(WY_MOVIE_URL, false),
                      MusicUtils::Algorithm::mdII(WY_MOVIE_DATA_URL, false).arg(id));
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    MusicObject::setSslConfiguration(&request);

    MusicSemaphoreLoop loop;
    QNetworkReply *reply = m_manager->post(request, parameter);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    if(!reply || reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    QJson::Parser parser;
    bool ok;
    const QVariant &data = parser.parse(reply->readAll(), &ok);
    if(ok)
    {
        QVariantMap value = data.toMap();
        if(value.contains("code") && value["code"].toInt() == 200)
        {
            value = value["data"].toMap();
            MusicObject::MusicSongInformation musicInfo;
            musicInfo.m_songId = QString::number(id);
            musicInfo.m_songName = MusicUtils::String::illegalCharactersReplaced(value["name"].toString());
            musicInfo.m_singerName = MusicUtils::String::illegalCharactersReplaced(value["artistName"].toString());
            musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt());

            value = value["brs"].toMap();
            for(const QString &key : value.keys())
            {
                int bitRate = key.toInt();
                MusicObject::MusicSongAttribute attr;
                if(bitRate <= 375)
                    attr.m_bitrate = MB_250;
                else if(bitRate > 375 && bitRate <= 625)
                    attr.m_bitrate = MB_500;
                else if(bitRate > 625 && bitRate <= 875)
                    attr.m_bitrate = MB_750;
                else if(bitRate > 875)
                    attr.m_bitrate = MB_1000;

                attr.m_url = value[key].toString();
                attr.m_format = MusicUtils::String::stringSplitToken(attr.m_url);
                if(!findUrlFileSize(&attr)) return;
                musicInfo.m_songAttrs.append(attr);
            }

            if(musicInfo.m_songAttrs.isEmpty())
            {
                return;
            }

            MusicSearchedItem item;
            item.m_songName = musicInfo.m_songName;
            item.m_singerName = musicInfo.m_singerName;
            item.m_time = musicInfo.m_timeLength;
            item.m_type = mapQueryServerString();
            Q_EMIT createSearchedItem(item);
            m_musicSongInfos << musicInfo;
        }
    }
}

void MusicWYQueryMovieRequest::getArtistMvsCount(qint64 id)
{
    if(!m_manager)
    {
        return;
    }

    m_pageTotal = DEFAULT_LEVEL_HIGHER;

    QNetworkRequest request;
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    const QByteArray &parameter = makeTokenQueryUrl(&request,
                      MusicUtils::Algorithm::mdII(WY_ARTIST_MOVIE_URL, false),
                      MusicUtils::Algorithm::mdII(WY_ARTIST_MOVIE_DATA_URL, false).arg(id).arg(0).arg(DEFAULT_LEVEL_HIGHER));
    if(!m_manager || m_stateCode != MusicObject::NetworkQuery) return;
    MusicObject::setSslConfiguration(&request);

    MusicSemaphoreLoop loop;
    QNetworkReply *reply = m_manager->post(request, parameter);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    if(!reply || reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    QJson::Parser parser;
    bool ok;
    const QVariant &data = parser.parse(reply->readAll(), &ok);
    if(ok)
    {
        const QVariantMap &value = data.toMap();
        if(value["code"].toInt() == 200 && value.contains("mvs"))
        {
            m_pageTotal = value["mvs"].toList().count();
        }
    }
}
