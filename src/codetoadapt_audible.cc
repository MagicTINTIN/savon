#include <boost/asio.hpp>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <iostream>
#include <fstream>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

using boost::asio::ip::udp;

enum
{
    max_length = 1024
};

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAME_SIZE = 1024 / 4; // 1024;

class server
{
public:
    server(boost::asio::io_context &io_context, short port, pa_simple *sAudioServer)
        : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
        do_receive(sAudioServer);
    }

    void do_receive(pa_simple *sAudioServer)
    {
        socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), sender_endpoint_,
            [this, sAudioServer](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    int error;
                    int16_t bufAudio[FRAME_SIZE * 2];

                    if (pa_simple_write(sAudioServer, data_, bytes_recvd, &error) < 0)
                    {
                        std::cerr << "pa_simple_write() failed: " << pa_strerror(error) << std::endl;
                        pa_simple_free(sAudioServer);
                        return;
                    }
                }
                do_receive(sAudioServer);
            });
    }

    void do_send(std::size_t length, pa_simple *sAudioServer)
    {
        socket_.async_send_to(
            boost::asio::buffer(data_, length), sender_endpoint_,
            [this, sAudioServer](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
            {
                do_receive(sAudioServer);
            });
    }

private:
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum
    {
        max_length = 1024
    };
    char data_[max_length];
};

int runClient(std::string &ip, std::string &port, char const *argv[], std::string &source)
{
    try
    {
        pa_sample_spec sample_spec;
        sample_spec.format = PA_SAMPLE_S16LE;
        sample_spec.rate = SAMPLE_RATE;
        sample_spec.channels = 2;

        pa_simple *sAudio = nullptr;
        int error;

        if (!(sAudio = pa_simple_new(nullptr, "Savon Capture < (Client)", PA_STREAM_RECORD, source.c_str(), "record", &sample_spec, nullptr, nullptr, &error)))
        {
            std::cerr << "Savon Client ERROR: pa_simple_new() failed: " << pa_strerror(error) << std::endl;
            return 1;
        }

        boost::asio::io_context io_context;

        udp::socket sSocket(io_context, udp::endpoint(udp::v4(), 0));

        udp::resolver resolver(io_context);
        udp::endpoint endpoint =
            *resolver.resolve(udp::v4(), argv[1], argv[2]).begin();

        std::cout << "Sending sound to server " << ip << ":" << port << std::endl;

        while (true)
        {
            int16_t bufAudio[FRAME_SIZE * 2];
            if (pa_simple_read(sAudio, bufAudio, sizeof(bufAudio), &error) < 0)
            {
                std::cerr << "pa_simple_read() failed: " << error << std::endl;
                pa_simple_free(sAudio);
                return 1;
            }

            sSocket.send_to(boost::asio::buffer(bufAudio, sizeof(bufAudio)), endpoint);
        }

        pa_simple_free(sAudio);
    }
    catch (std::exception &e)
    {
        std::cerr << "Client Exception: " << e.what() << std::endl;
        return 2;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    int endStatus(0);
    if (argc != 3)
    {
        std::cerr << "Usage: savon <host> <port>\nOr savon -s|--server <port>\n";
        return 1;
    }
    std::ifstream sourceFileFlux("./source.cfg");
    std::string audioSource;
    if (sourceFileFlux)
    {
        getline(sourceFileFlux, audioSource);
    }
    else
    {
        std::cout << "ERROR: Impossible to open ./source.cfg" << std::endl;
    }
    sourceFileFlux.close();
    std::vector<std::string> args(argv, argv + argc);
    if (args[1] == "-s" || args[1] == "--server")
    {
        std::cout << "Starting Savon server on port " << args[2] << "..." << std::endl;

        boost::asio::io_context io_context;

        pa_sample_spec sample_spec;
        sample_spec.format = PA_SAMPLE_S16LE;
        sample_spec.rate = SAMPLE_RATE;
        sample_spec.channels = 2;

        pa_simple *sAudioServer = nullptr;
        int error;

        if (!(sAudioServer = pa_simple_new(nullptr, "Savon Player < (Server)", PA_STREAM_PLAYBACK, nullptr, "playback", &sample_spec, nullptr, nullptr, &error)))
        {
            std::cerr << "pa_simple_new() failed: " << pa_strerror(error) << std::endl;
            return 1;
        }

        server s(io_context, std::atoi(argv[2]), sAudioServer);

        io_context.run();

        pa_simple_free(sAudioServer);
    }
    else
    {
        std::cout << "Starting Savon client..." << std::endl;
        endStatus = runClient(args[1], args[2], argv, audioSource);
    }

    return endStatus;
}