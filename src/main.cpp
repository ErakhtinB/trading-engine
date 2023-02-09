#include <future>
#include <iostream>
#include <shared_mutex>
#include <thread>

#include <binapi/api.hpp>
#include <binapi/websocket.hpp>

#include <boost/asio/io_context.hpp>

using namespace std::chrono_literals;
static constexpr auto ExpirationTimeout = 60s;
static constexpr auto AfterExpirationTimeout = 30s;

static constexpr auto traidingPair = "BNBUSDT";
static constexpr auto traidingAmount = "0.001";

static constexpr auto percentageAbove = 0.25;
static constexpr auto percentageBelow = 0.25;

static void SellOrder(boost::asio::io_context& ioctx, binapi::rest::api& api)
{
    api.new_order(
        traidingPair,
        binapi::e_side::sell,
        binapi::e_type::market,
        binapi::e_time::GTC,
        binapi::e_trade_resp_type::FULL,
        traidingAmount,
        "",
        "",
        "",
        "",
        [](const char *fl, int ec, std::string emsg, auto res)
        {
            if ( ec )
            {
                std::cerr << "post place new order error: fl=" << fl << ", ec=" << ec << ", emsg=" << emsg << std::endl;
                return false;
            }
            return true;
        });
}

static void BuyOrder(boost::asio::io_context& ioctx, binapi::rest::api& api,
    std::atomic<bool>& tradeIsReady, binapi::double_type& maxPrice, binapi::double_type& minPrice)
{
    api.new_order(
        traidingPair,
        binapi::e_side::buy,
        binapi::e_type::market,
        binapi::e_time::GTC,
        binapi::e_trade_resp_type::FULL,
        traidingAmount,
        "",
        "",
        "",
        "",
        [&tradeIsReady, &maxPrice, &minPrice](const char *fl, int ec, std::string emsg, auto res)
        {
            if ( ec )
            {
                std::cerr << "post place new order error: fl=" << fl << ", ec=" << ec << ", emsg=" << emsg << std::endl;
                return false;
            }
            const auto resPrice = res.get_responce_full().fills[0].price;
            maxPrice = resPrice * (1.0 + percentageAbove / 100.0);
            minPrice = resPrice * (1.0 - percentageBelow / 100.0);
            tradeIsReady = true;
            return true;
        });
}

int main() {
    auto expired = false;
    auto firstTake = true;
    while (1)
    {
        if (expired)
        {
            std::this_thread::sleep_for(AfterExpirationTimeout);
        }
        else if (!firstTake)
        {
            break;
        }
        firstTake = false;

        boost::asio::io_context ioctx;

        binapi::rest::api api(
            ioctx
            ,"testnet.binance.vision"
            ,"443"
            ,"ek9zLITdNni2O6hf1kgqJHaYXmDVfWSHgr5itUjaYEKKRHcZAwqoTo6a8U2Pkrz8"
            ,"MDbE1hdAX9JvdS7mQw1xABa6JN5lRyrXJfveixH37thcoe54spDPTwWLgUc2PmhO"
            ,10000
        );

        // open web socket
        binapi::ws::websockets ws{
            ioctx
            , "testnet.binance.visions"
            , "9443"
        };

        binapi::double_type minPrice = {}, maxPrice = {};
        std::atomic<bool> tradeIsReady = {};

        BuyOrder(ioctx, api, tradeIsReady, maxPrice, minPrice);

        auto expired = false, executed = false;
        std::shared_mutex expiredMutex;
        binapi::ws::websockets::handle book_handler = {};
        book_handler = ws.book(traidingPair,
            [&](const char *fl, int ec, std::string emsg, auto book)
            {
                if ( ec )
                {
                    std::cerr << "subscribe book error: fl=" << fl << ", ec=" << ec << ", emsg=" << emsg << std::endl;
                    return false;
                }
                if (tradeIsReady)
                {
                    std::shared_lock lock(expiredMutex);
                    if (expired)
                    {
                        ws.unsubscribe(book_handler);
                    }
                    else if (book.b > maxPrice || book.b < minPrice)
                    {
                        SellOrder(ioctx, api);
                        executed = true;
                        ws.unsubscribe(book_handler);
                    }
                }
                return true;
            });
        auto _ = std::async(std::launch::async, [&]()
        {
            while (!tradeIsReady);
            const auto begin = std::chrono::system_clock::now();
            auto now = begin;
            while (now - begin < ExpirationTimeout)
            {
                if (executed)
                {
                    return;
                }
                now = std::chrono::system_clock::now();
            }
            std::unique_lock lock(expiredMutex);
            expired = true;
            if (!executed)
            {
                SellOrder(ioctx, api);
                executed = true;
            }
        });

        ioctx.run();
    }

    return EXIT_SUCCESS;
}
