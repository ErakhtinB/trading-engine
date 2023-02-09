
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of binapi(https://github.com/niXman/binapi) project.
//
// Copyright (c) 2019-2021 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#include <binapi/api.hpp>
#include <binapi/websocket.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <iostream>

int main() {
    boost::asio::io_context ioctx;

    binapi::ws::websockets ws{
        ioctx
        // , "testnet.binance.vision"
        // , "9443"
        ,"stream.binance.com"
        ,"9443"
    };

    // post buy market order
	// BinaCPP::send_order( "BNBETH", "BUY", "MARKET", "GTC", 20 , 0,   "",0,0, recvWindow, result );

    binapi::ws::websockets::handle book_handler = {};
    book_handler = ws.book("BNBUSDT",
        [&book_handler, &ws](const char *fl, int ec, std::string emsg, auto book)
        {
            if ( ec )
            {
                std::cerr << "subscribe book error: fl=" << fl << ", ec=" << ec << ", emsg=" << emsg << std::endl;
                return false;
            }
            std::cout << book << std::endl;
            return true;
            // we need b
            // if ( trashhold)
            // book["b"], (if we sell)
            // if (true)
            // {
            //     // send
            //     ws.unsubscribe(book_handler);
            // }
        });

    ioctx.run();

    return EXIT_SUCCESS;
}
