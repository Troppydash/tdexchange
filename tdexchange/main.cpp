#include <iostream>
#include <string>
#include <format>

#include <nlohmann/json.hpp>

//#include <boost/asio.hpp>
//#include <boost/array.hpp>

#include "logger.h"
//
//auto start_server() -> void
//{
//	using boost::asio::ip::tcp;
//	using namespace std;
//
//	try
//	{
//		boost::asio::io_context context;
//
//		int port = 1234;
//		tcp::acceptor accepter(context, tcp::endpoint(tcp::v4(), port));
//		cout << "server started on port " << port << endl;
//
//		// connection loop
//		for (;;)
//		{
//			tcp::socket socket(context);
//			accepter.accept(socket);
//
//			// read message
//			boost::array<char, 1024> buf;
//			boost::system::error_code error;  // passed as reference to get potential errors
//
//			while (true)
//			{
//				// read bytes
//				size_t read = socket.read_some(boost::asio::buffer(buf));
//				cout << "read " << read << " bytes" << endl;
//				std::string message(buf.data(), buf.data() + read);
//
//
//				// send bytes
//				size_t sent = socket.send(boost::asio::buffer(message));
//				cout << "transferred " << sent << " bytes" << endl;
//
//				if (message == "bye")
//				{
//					break;
//				}
//			}
//
//			socket.close();
//		}
//	}
//	catch (const std::exception &ex)
//	{
//		cerr << ex.what() << endl;
//	}
//}

#include "exchange.h"

//using namespace nlohmann;
//using namespace nlohmann::json_literals;

auto main() -> int
{
	//json test = "[1, 2, \"hi\"]"_json;
	//std::cout << test << "\n";
	//std::cout << test.size() << "\n";
	//std::cout << test[2] << "\n";

	//return 0;

	//

	market::exchange exch;

	exch.user_order(market::side::BID, 1, 1, 1000, 10);
	exch.user_order(market::side::ASK, 1, 1, 1010, 10);
	std::cout << exch.repr_tickers();

	exch.user_order(market::side::BID, 2, 1, 1020, 15, true);
	std::cout << exch.repr_tickers();

	//exch.user_order(market::side::BID, 2, 1, 1012, 10);
	//std::cout << exch.repr_tickers();

	//exch.user_order(market::side::ASK, 1, 1, 1011, 10);
	//std::cout << exch.repr_tickers();

	//exch.user_cancel(1);
	//std::cout << exch.repr_tickers();


	//std::cout << exch.repr_users();
	std::cout << exch.repr_transactions();

	//std::string test = "test";
	//logger::log(std::format("Name {}", test), logger::mode::WARN);
	//start_server();

	return 0;
}