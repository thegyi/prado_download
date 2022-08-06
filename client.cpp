#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/regex.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
  try {
    auto const host = "www.consilium.europa.eu";
    auto const port = "443";
    auto const target = "/prado/hu/search-by-document-title.html";
    int version = 11;

    net::io_context ioc;

    ssl::context ctx(ssl::context::tlsv12_client);

    ctx.set_verify_mode(ssl::verify_none);

    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host)) {
      beast::error_code ec{static_cast<int>(::ERR_get_error()),
                           net::error::get_ssl_category()};
      throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(host, port);

    beast::get_lowest_layer(stream).connect(results);

    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::get, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;

    http::read(stream, buffer, res);
    std::ofstream savedHtmlPage("prado_documents.html");
    std::string htmlContent =
        boost::beast::buffers_to_string(res.body().data());
    savedHtmlPage << htmlContent;
    savedHtmlPage.close();

    boost::regex expr{"[A-Z]{3}-[A-Z|0-9]{2}-[0-9]{5}/index.html"};
    boost::regex_token_iterator<std::string::iterator> it{
        htmlContent.begin(), htmlContent.end(), expr};
    boost::regex_token_iterator<std::string::iterator> end;
    std::ofstream countryCodes("documents.txt");
    while (it != end) {
      countryCodes << *it << std::endl;
      http::request<http::string_body> req{http::verb::get, "/prado/hu/" + *it,
                                           version};
      req.set(http::field::host, host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

      http::write(stream, req);
      http::response<http::dynamic_body> res;
      beast::flat_buffer buffer;
      http::read(stream, buffer, res);
      std::string fn = it->str().substr(0, 12);
      std::ofstream page("pages/" + fn + ".html");
      std::string pageContent =
          boost::beast::buffers_to_string(res.body().data());
      page << pageContent;
      page.close();
      std::cout << *it << std::endl;
      it++;

      std::this_thread::sleep_for(10s);
    }

    countryCodes.close();

    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) {
      ec = {};
    }
    if (ec)
      throw beast::system_error{ec};

  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
