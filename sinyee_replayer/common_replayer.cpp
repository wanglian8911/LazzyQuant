#include <QPair>

#include "common_replayer.h"

CommonReplayer::CommonReplayer(QObject *parent) : QObject(parent)
{
}

void CommonReplayer::sortTickPairList()
{
    std::stable_sort(tickPairList.begin(), tickPairList.end(), [](const auto &item1, const auto &item2) -> bool {
        return item1.second.timestamp < item2.second.timestamp;
    });
    tickCnt = tickPairList.size();
    replayIdx = 0;
}

/*!
 * \brief CommonReplayer::startReplay
 * 复盘指定日期的行情, 复盘合约由配置文件中ReplayList指定.
 *
 * \param date 复盘日期.
 */
void CommonReplayer::startReplay(const QString &date)
{
    startReplay(date, replayList);
}

/*!
 * \brief CommonReplayer::startReplay
 * 复盘指定日期的单个合约行情, 复盘合约由参数instrument指定.
 *
 * \param date 复盘日期.
 * \param instrument 复盘合约.
 */
void CommonReplayer::startReplay(const QString &date, const QString &instrument)
{
    startReplay(date, QStringList() << instrument);
}

/*!
 * \brief CommonReplayer::startReplay
 * 复盘指定日期的多个合约行情, 复盘合约列表由参数instruments指定.
 *
 * \param date 复盘日期.
 * \param instruments 复盘合约列表.
 */
void CommonReplayer::startReplay(const QString &date, const QStringList &instruments)
{
    prepareReplay(date, instruments);
    replayTo(INT_MAX);
}

bool CommonReplayer::prepareReplay(const QString &date)
{
    return prepareReplay(date, replayList);
}

bool CommonReplayer::prepareReplay(const QString &date, const QStringList &instruments)
{
    replayDate = date;
    tickPairList.clear();
    sumVol.clear();
    for (const auto &instrument : instruments) {
        appendTicksToList(date, instrument);
    }
    sortTickPairList();
    if (tickCnt > 0) {
        emit tradingDayChanged(date);
        mapTime.setTradingDay(date);
    }
    return tickCnt > 0;
}

bool CommonReplayer::replayTo(int time)
{
    bool ret = false;
    if (tickCnt > 0) {
        for (; replayIdx < tickCnt; replayIdx++) {
            const auto &item = tickPairList[replayIdx];
            const auto &tick = item.second;
            if (time >= tick.getTime()) {
                int emitTime = tick.getTime() % 86400;
                sumVol[item.first] += tick.volume;
                auto hour = emitTime / 3600;
                if (hour < 8) {
                    emitTime -= (4 * 3600);
                    if (emitTime < 0) {
                        emitTime += 86400;
                    }
                }
                emit newMarketData(item.first,
                                   mapTime(emitTime),
                                   tick.price,
                                   sumVol[item.first],
                                   tick.askPrice,
                                   tick.askVolume,
                                   tick.bidPrice,
                                   tick.bidVolume);
                ret = true;
            } else {
                break;
            }
        }
        if (replayIdx >= tickCnt) {
            emit endOfReplay(replayDate);
            tickCnt = 0;
        }
    }
    return ret;
}
