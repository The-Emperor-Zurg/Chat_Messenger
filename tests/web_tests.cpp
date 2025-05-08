// #pragma once
// #define NOMINMAX
// #include <windows.h>
// #undef min
// #undef max

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define BOOST_TEST_MODULE WebTests
#include <boost/test/included/unit_test.hpp>
#include <boost/format.hpp>

#include "../web/lib/server.h"
#include "../web/lib/commands.h"

using tcp = ip::tcp;
namespace websocket = beast::websocket;

struct MessageWs {
    OutCommand code;
    std::string message;
};

struct TestClient {
    explicit TestClient(asio::io_context& ioc)
        : resolver_(ioc), ws_(ioc)
    {}

    void connect() {
        auto const results = resolver_.resolve("localhost", "8080");
        asio::connect(ws_.next_layer(), results.begin(), results.end());
        ws_.handshake("localhost", "/");
    }

    void disconnect() {
        ws_.close(websocket::close_code::normal);
    }

    void sendMessage(InCommand cmd, const std::string& payload = "") {
        std::string msg = str(boost::format("%d%s") % int(cmd) % (payload.empty() ? "" : " " + payload));
        ws_.write(asio::buffer(msg));
    }

    MessageWs receiveMessage() {
        auto [code, text] = receiveRaw();
        return {code, text};
    }

    ErrorCode receiveError() {
        auto [code, text] = receiveRaw();
        BOOST_CHECK(code == OutCommand::ERRORR);
        return static_cast<ErrorCode>(std::stoi(text));
    }

private:
    std::pair<OutCommand, std::string> receiveRaw() {
        beast::flat_buffer buffer;
        ws_.read(buffer);
        auto data = beast::buffers_to_string(buffer.data());
        auto pos = data.find(' ');
        if (pos != std::string::npos) {
            int c = std::stoi(data.substr(0, pos));
            return {static_cast<OutCommand>(c), data.substr(pos + 1)};
        }

        return {static_cast<OutCommand>(std::stoi(data)), std::string{}};
    }

    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
};

std::vector<std::tuple<std::string, std::string, std::string>> parseHistory(const std::string& payload) {
    std::vector<std::tuple<std::string, std::string, std::string>> out;
    if (payload.empty()) return out;

    std::stringstream ss(payload);
    std::string entry;
    while (std::getline(ss, entry, '|')) {
        auto p1 = entry.find(';');
        auto p2 = entry.find(';', p1+1);

        std::string id     = entry.substr(0, p1);
        std::string author = entry.substr(p1+1, p2-p1-1);
        std::string text   = entry.substr(p2+1);

        out.emplace_back(id, author, text);
    }

    return out;
}

struct WsTestGlobalFixture {
    WsTestGlobalFixture()
        : server(2, 8080)
    {
        thread = std::thread([this]{ server.start(); });
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ~WsTestGlobalFixture() {
        server.stop();
        thread.join();
    }

    Server server;
    std::thread thread;
};

struct WsTestFixture {
    WsTestFixture()
        : client1(ioc), client2(ioc)
    {}

    void connectClients() {
        client1.connect();
        clientId1 = client1.receiveMessage().message;

        client2.connect();
        clientId2 = client2.receiveMessage().message;
    }

    asio::io_context ioc;
    TestClient client1;
    TestClient client2;

    std::string clientId1;
    std::string clientId2;
};

BOOST_GLOBAL_FIXTURE(WsTestGlobalFixture);

BOOST_FIXTURE_TEST_CASE(RenameUserSuccess, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::RENAME_USER, "NewName");
    auto resp = client1.receiveMessage();
    BOOST_CHECK(resp.code == OutCommand::USER_RENAMED);

    client1.sendMessage(InCommand::RENAME_USER);
    auto err = client1.receiveError();
    BOOST_CHECK(err == ErrorCode::INCORRECT_FORMAT);
}

BOOST_FIXTURE_TEST_CASE(CreatePersonalChatSuccessAndFailure, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " ChatWithMe");
    auto msg = client1.receiveMessage();
    BOOST_CHECK(msg.code == OutCommand::CHAT_CREATED);
    auto chatId = msg.message;

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, "invalid-uuid ChatX");
    auto err = client1.receiveError();
    BOOST_CHECK(err == ErrorCode::INCORRECT_FORMAT);
}

BOOST_FIXTURE_TEST_CASE(CreateOpenAndCloseGroup, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_OPEN_GROUP, "OpenChat");
    auto openResp = client1.receiveMessage();
    BOOST_CHECK(openResp.code == OutCommand::CHAT_CREATED);
    auto openId = openResp.message;

    client1.sendMessage(InCommand::CREATE_CLOSE_GROUP, "CloseChat");
    auto closeResp = client1.receiveMessage();
    BOOST_CHECK(closeResp.code == OutCommand::CHAT_CREATED);
    auto closeId = closeResp.message;
}

BOOST_FIXTURE_TEST_CASE(DeleteChatByAdminAndOthers, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " TestChat");
    auto cr = client1.receiveMessage();
    std::string chatId = cr.message;

    client1.sendMessage(InCommand::DELETE_CHAT, chatId);
    auto ok = client1.receiveMessage();
    BOOST_CHECK(ok.code == OutCommand::CHAT_DELETED);

    client1.sendMessage(InCommand::DELETE_CHAT, chatId);
    auto err2 = client1.receiveError();
    BOOST_CHECK(err2 == ErrorCode::ERROR_CHAT_DELETE);
}

BOOST_FIXTURE_TEST_CASE(PersonalChatParticipantManagementFails, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " ChatX");
    auto cr = client1.receiveMessage();
    std::string chatId = cr.message;

    client1.sendMessage(InCommand::ADD_PARTICIPANT, chatId + " " + clientId2);
    auto errAdd = client1.receiveError();
    BOOST_CHECK(errAdd == ErrorCode::ERROR_PARTICIPANT_ADD);

    client1.sendMessage(InCommand::REMOVE_PARTICIPANT, chatId + " " + clientId2);
    auto errRem = client1.receiveError();
    BOOST_CHECK(errRem == ErrorCode::ERROR_PARTICIPANT_REMOVE);
}

BOOST_FIXTURE_TEST_CASE(ParticipantManagementInOpenGroup, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_OPEN_GROUP, "Group1");
    auto grpResp = client1.receiveMessage();
    std::string grpId = grpResp.message;

    client1.sendMessage(InCommand::ADD_PARTICIPANT, grpId + " " + clientId2);
    auto addResp = client1.receiveMessage();
    BOOST_CHECK(addResp.code == OutCommand::PARTICIPANT_ADDED);

    client1.sendMessage(InCommand::REMOVE_PARTICIPANT, grpId + " " + clientId2);
    auto remResp = client1.receiveMessage();
    BOOST_CHECK(remResp.code == OutCommand::PARTICIPANT_REMOVED);

    client1.sendMessage(InCommand::REMOVE_PARTICIPANT, grpId + " " + clientId2);
    auto err = client1.receiveError();
    BOOST_CHECK(err == ErrorCode::ERROR_PARTICIPANT_REMOVE);
}

BOOST_FIXTURE_TEST_CASE(ParticipantManagementInCloseGroup, WsTestFixture) {
    connectClients();

    TestClient client3{ioc};
    client3.connect();
    std::string clientId3 = client3.receiveMessage().message;

    client1.sendMessage(InCommand::CREATE_CLOSE_GROUP, "Secret");
    auto cr = client1.receiveMessage();
    std::string cid = cr.message;

    client1.sendMessage(InCommand::ADD_PARTICIPANT, cid + " " + clientId2);
    auto a1 = client1.receiveMessage();
    BOOST_CHECK(a1.code == OutCommand::PARTICIPANT_ADDED);

    client2.sendMessage(InCommand::ADD_PARTICIPANT, cid + " " + clientId3);
    auto e1 = client2.receiveError();
    BOOST_CHECK(e1 == ErrorCode::ERROR_PARTICIPANT_ADD);

    client1.sendMessage(InCommand::ADD_PARTICIPANT, cid + " " + clientId3);
    auto a2 = client1.receiveMessage();
    BOOST_CHECK(a2.code == OutCommand::PARTICIPANT_ADDED);

    client2.sendMessage(InCommand::REMOVE_PARTICIPANT, cid + " " + clientId3);
    auto e2 = client2.receiveError();
    BOOST_CHECK(e2 == ErrorCode::ERROR_PARTICIPANT_REMOVE);

    client1.sendMessage(InCommand::REMOVE_PARTICIPANT, cid + " " + clientId3);
    auto r2 = client1.receiveMessage();
    BOOST_CHECK(r2.code == OutCommand::PARTICIPANT_REMOVED);
}

BOOST_FIXTURE_TEST_CASE(MessageSendSuccessAndFailure, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " MChat");
    auto cr = client1.receiveMessage();
    std::string mid = cr.message;

    client1.sendMessage(InCommand::SEND_MESSAGE, mid + " Hello");
    auto ok = client1.receiveMessage();
    BOOST_CHECK(ok.code == OutCommand::MESSAGE_SENT);

    std::string fake = "77777777-7777-7777-0000-000000000000";
    client1.sendMessage(InCommand::SEND_MESSAGE, fake + " Hi");
    auto ef = client1.receiveError();
    BOOST_CHECK(ef == ErrorCode::ERROR_SEND_MESSAGE);
}

BOOST_FIXTURE_TEST_CASE(MessageEditSuccessAndFailure, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " EChat");
    auto cr = client1.receiveMessage();
    BOOST_CHECK(cr.code == OutCommand::CHAT_CREATED);
    std::string cid = cr.message;

    client1.sendMessage(InCommand::SEND_MESSAGE, cid + " Original");
    auto sent = client1.receiveMessage();
    BOOST_CHECK(sent.code == OutCommand::MESSAGE_SENT);

    client1.sendMessage(InCommand::GET_HISTORY, cid);
    auto h0 = client1.receiveMessage();
    BOOST_CHECK(h0.code == OutCommand::HISTORY);
    auto vec0 = parseHistory(h0.message);
    BOOST_REQUIRE(vec0.size() == 1);
    std::string msgId = std::get<0>(vec0[0]);

    client1.sendMessage(InCommand::EDIT_MESSAGE, cid + " " + msgId + " Edited");
    auto ed = client1.receiveMessage();
    BOOST_CHECK(ed.code == OutCommand::MESSAGE_EDITED);

    client1.sendMessage(InCommand::EDIT_MESSAGE, cid + " bad-id NewText");
    auto e2 = client1.receiveError();
    BOOST_CHECK(e2 == ErrorCode::INCORRECT_FORMAT);
}

BOOST_FIXTURE_TEST_CASE(MessageRemoveSuccessAndFailure, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " RChat");
    auto cr = client1.receiveMessage();
    BOOST_CHECK(cr.code == OutCommand::CHAT_CREATED);
    std::string cid = cr.message;

    client1.sendMessage(InCommand::SEND_MESSAGE, cid + " ToDelete");
    auto sent = client1.receiveMessage();
    BOOST_CHECK(sent.code == OutCommand::MESSAGE_SENT);

    client1.sendMessage(InCommand::GET_HISTORY, cid);
    auto h0 = client1.receiveMessage();
    BOOST_CHECK(h0.code == OutCommand::HISTORY);
    auto vec0 = parseHistory(h0.message);
    BOOST_REQUIRE(vec0.size() == 1);
    std::string msgId = std::get<0>(vec0[0]);

    client1.sendMessage(InCommand::REMOVE_MESSAGE, cid + " " + msgId);
    auto rm = client1.receiveMessage();
    BOOST_CHECK(rm.code == OutCommand::MESSAGE_REMOVED);

    client1.sendMessage(InCommand::REMOVE_MESSAGE, cid + " " + msgId);
    auto e2 = client1.receiveError();
    BOOST_CHECK(e2 == ErrorCode::ERROR_REMOVE_MESSAGE);
}

BOOST_FIXTURE_TEST_CASE(HistoryView, WsTestFixture) {
    connectClients();

    client1.sendMessage(InCommand::CREATE_PERSONAL_CHAT, clientId2 + " HChat");
    auto cr = client1.receiveMessage();
    BOOST_CHECK(cr.code == OutCommand::CHAT_CREATED);
    std::string cid = cr.message;

    client1.sendMessage(InCommand::GET_HISTORY, cid);
    auto h0 = client1.receiveMessage();
    BOOST_CHECK(h0.code == OutCommand::HISTORY);
    auto vec0 = parseHistory(h0.message);
    BOOST_CHECK(vec0.empty());

    client1.sendMessage(InCommand::SEND_MESSAGE, cid + " First");
    client1.receiveMessage();
    client1.sendMessage(InCommand::SEND_MESSAGE, cid + " Second");
    client1.receiveMessage();

    client1.sendMessage(InCommand::GET_HISTORY, cid);
    auto h1 = client1.receiveMessage();
    BOOST_CHECK(h1.code == OutCommand::HISTORY);
    auto vec1 = parseHistory(h1.message);
    BOOST_CHECK(vec1.size() == 2);
    BOOST_CHECK(std::get<2>(vec1[0]) == "First");
    BOOST_CHECK(std::get<2>(vec1[1]) == "Second");
}

BOOST_FIXTURE_TEST_CASE(UnknownAndBadFormatCommands, WsTestFixture) {
    connectClients();

    client1.sendMessage(static_cast<InCommand>(999), "");
    auto err1 = client1.receiveError();
    BOOST_CHECK(err1 == ErrorCode::UNKNOWN_COMMAND);
}
