/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ChannelMgr.h"
#include "Player.h"
#include "GameConfig.h"
#include "StringConvert.h"
#include "Tokenize.h"

ChannelMgr::~ChannelMgr()
{
    for (ChannelMap::iterator itr = channels.begin(); itr != channels.end(); ++itr)
        delete itr->second;

    channels.clear();
}

ChannelMgr* ChannelMgr::forTeam(TeamId teamId)
{
    static ChannelMgr allianceChannelMgr(TEAM_ALLIANCE);
    static ChannelMgr hordeChannelMgr(TEAM_HORDE);

    if (sGameConfig->GetBoolConfig("AllowTwoSide.Interaction.Channel"))
        return &allianceChannelMgr;        // cross-faction

    if (teamId == TEAM_ALLIANCE)
        return &allianceChannelMgr;

    if (teamId == TEAM_HORDE)
        return &hordeChannelMgr;

    return nullptr;
}

void ChannelMgr::LoadChannels()
{
    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    //                                                    0          1     2     3         4          5
    QueryResult result = CharacterDatabase.PQuery("SELECT channelId, name, team, announce, ownership, password FROM channels WHERE team = %u ORDER BY channelId ASC", _teamId);
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 channels for %s", _teamId == TEAM_ALLIANCE ? "Alliance" : "Horde");
        LOG_WARN("sql.sql", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        if (!fields)
            break;

        uint32 channelDBId = fields[0].GetUInt32();
        std::string channelName = fields[1].GetString();
        std::string password = fields[5].GetString();
        std::wstring channelWName;
        Utf8toWStr(channelName, channelWName);

        Channel* newChannel = new Channel(channelName, 0, channelDBId, TeamId(fields[2].GetUInt32()), fields[3].GetUInt8(), fields[4].GetUInt8());
        newChannel->SetPassword(password);
        channels[channelWName] = newChannel;

        if (QueryResult banResult = CharacterDatabase.PQuery("SELECT playerGUID, banTime FROM channels_bans WHERE channelId = %u", channelDBId))
        {
            do
            {
                Field* banFields = banResult->Fetch();
                if (!banFields)
                    break;
                newChannel->AddBan(banFields[0].GetUInt32(), banFields[1].GetUInt32());
            } while (banResult->NextRow());
        }

        if (channelDBId > ChannelMgr::_channelIdMax)
            ChannelMgr::_channelIdMax = channelDBId;
        ++count;
    } while (result->NextRow());

    LOG_INFO("channel", ">> Loaded %u channels for %s in %ums", count, _teamId == TEAM_ALLIANCE ? "Alliance" : "Horde", GetMSTimeDiffToNow(oldMSTime));
}

Channel* ChannelMgr::GetJoinChannel(std::string const& name, uint32 channelId)
{
    std::wstring wname;
    Utf8toWStr(name, wname);
    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(wname);

    if (i == channels.end())
    {
        Channel* nchan = new Channel(name, channelId, 0, _teamId);
        channels[wname] = nchan;
        return nchan;
    }

    return i->second;
}

Channel* ChannelMgr::GetChannel(std::string const& name, Player* player, bool pkt)
{
    std::wstring wname;
    Utf8toWStr(name, wname);
    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(wname);

    if (i == channels.end())
    {
        if (pkt)
        {
            WorldPacket data;
            MakeNotOnPacket(&data, name);
            player->GetSession()->SendPacket(&data);
        }

        return nullptr;
    }

    return i->second;
}


uint32 ChannelMgr::_channelIdMax = 0;
ChannelMgr::ChannelRightsMap ChannelMgr::channels_rights;
ChannelRights ChannelMgr::channelRightsEmpty;

void ChannelMgr::LoadChannelRights()
{
    uint32 oldMSTime = getMSTime();
    channels_rights.clear();

    QueryResult result = CharacterDatabase.Query("SELECT name, flags, joinmessage, delaymessage, moderators FROM channels_rights");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 Channel Rights!");
        LOG_WARN("sql.sql", "");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        std::set<uint32> moderators;
        const char* moderatorList = fields[4].GetCString();
        if (moderatorList)
        {
            for (auto const& accountID : Warhead::Tokenize(moderatorList, ' ', false))
            {
                uint64 moderator_acc = Warhead::StringTo<uint64>(accountID).value_or(0);
                if (moderator_acc && ((uint32)moderator_acc) == moderator_acc)
                    moderators.insert((uint32)moderator_acc);
            }
        }

        SetChannelRightsFor(fields[0].GetString(), fields[1].GetUInt32(), fields[2].GetString(), fields[3].GetString(), moderators);

        ++count;
    } while (result->NextRow());

    LOG_INFO("channel", ">> Loaded %d Channel Rights in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("channel", "");
}

const ChannelRights& ChannelMgr::GetChannelRightsFor(const std::string& name)
{
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    ChannelRightsMap::const_iterator itr = channels_rights.find(nameStr);
    if (itr != channels_rights.end())
        return itr->second;
    return channelRightsEmpty;
}

void ChannelMgr::SetChannelRightsFor(const std::string& name, const uint32& flags, const std::string& joinmessage, const std::string& speakmessage, const std::set<uint32>& moderators)
{
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    channels_rights[nameStr] = ChannelRights(flags, joinmessage, speakmessage, moderators);
}

void ChannelMgr::MakeNotOnPacket(WorldPacket* data, std::string const& name)
{
    data->Initialize(SMSG_CHANNEL_NOTIFY, 1 + name.size());
    (*data) << uint8(5) << name;
}
