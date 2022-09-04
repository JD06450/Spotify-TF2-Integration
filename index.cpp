#include "index.hpp"

#include "base64.hpp"
#include "curl-util.hpp"
#include "tf2-logfile.hpp"
#include "spotify-api/spotify-api.hpp"

using namespace std;
namespace api = spotify_api;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpResponse = shared_ptr<HttpServer::Response>;
using HttpRequest = shared_ptr<HttpServer::Request>;

void parse_command(const string &command);

string access_token;

api::Api_Session *session;

int main()
{
    fstream fs;
    int env_vars = 2;
    string client_id;
    string client_secret;
    char auth_token[64];
    char buffer[150];
    char buffer_2[50];

    fs.open("../.env", fstream::in);

    for (int i = 0; i < env_vars; i++)
    {
        if (fs.bad())
        {
            printf("File operation failed");
            return 1;
        }
        fs.getline(buffer, 150, '=');

        fs.getline(buffer_2, 50);

        if (!strcmp(buffer, "CLIENT_ID"))
        {
            if (regex_search(buffer_2, regex("[^A-Fa-f0-9]")) || strnlen(buffer_2, 50) > 32)
            {
                cout << "Environment variable invalid: " << buffer << endl;
                return 1;
            }
            else
            {
                client_id.assign(buffer_2);
            }
        }
        else if (!strcmp(buffer, "CLIENT_SECRET"))
        {
            if (regex_search(buffer_2, regex("[^A-Fa-f0-9]")) || strnlen(buffer_2, 50) > 32)
            {
                cout << "Environment variable invalid: " << buffer << endl;
                return 1;
            }
            else
            {
                client_secret.assign(buffer_2);
            }
        }
    }

    string auth_code;
    string auth_base64 = base64_encode(string(client_id + ':' + client_secret), false);

    // Written using sample code

    HttpServer server;
    server.config.port = 8080;

    server.resource["^/$"]["GET"] = [&auth_code](HttpResponse res, HttpRequest req)
    {
        string content = req->query_string;
        // cout << content << endl;
        string buffer1;
        string buffer2;
        size_t delimPos1;
        size_t delimPos2;
        size_t i = 0;
        while (i < content.length())
        {
            delimPos1 = content.find_first_of('=', i);
            delimPos2 = content.find_first_of('&', i);
            if (!i && delimPos1 == string::npos)
            {
                puts("Invalid query string");
                return;
            }

            buffer1 = content.substr(i, delimPos1);
            buffer2 = content.substr(delimPos1 + 1, delimPos2 - delimPos1);
            // cout << buffer1 << ": " << buffer2 << endl;

            if (!strcmp(buffer1.c_str(), "code"))
            {
                auth_code = buffer2;
                cout << "found code" << endl;
                break;
            }
            i = delimPos2 + 1;
        }

        res->write("You can now close this window.");
    };

    server.on_error = [](shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/)
    {
        // Handle errors here
        // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
    };

    promise<unsigned short> server_port;
    thread *server_thread = new thread([&server, &server_port]()
                                       { server.start([&server_port](unsigned short port)
                                                      { server_port.set_value(port); }); });

    cout << "Simple Web Server listening on port " << server_port.get_future().get() << endl;

    string query_string = "client_id=" + client_id +
                          "&response_type=code" +
                          "&redirect_uri=" + http::url_encode("http://localhost:8080") +
                          "&scope=" + http::url_encode("user-read-currently-playing user-read-playback-state user-modify-playback-state playlist-modify-private playlist-read-private");

    system((string("firefox --window-size 640,480 --new-window \"accounts.spotify.com/authorize?") + query_string + '\"').c_str());

    while (auth_code == "")
    {
        sleep(1);
    }
    // Kill the webserver as it is no longer needed
    server_thread->detach();
    delete server_thread;

    cout << "Simple Web Server destroyed." << endl;

    session = api::start_spotify_session(auth_code, "http://localhost:8080", auth_base64);

    api::track *current_track = session->get_currently_playing_track();

    printf("Currently Playing: %s by %s\nFrom album: %s\n", current_track->name.c_str(), current_track->artists[0]->name.c_str(), current_track->album->name.c_str());

    api::album *song_album = session->get_album(current_track->album->id);

    if (song_album->album_type == "Single")
    {
        puts("\n Album is a single");
    }
    else
    {
        puts("\nSongs in current album:");
        for (auto track = song_album->tracks.items.begin(); track != song_album->tracks.items.end(); ++track)
        {
            printf("%s\n", (*track)->name.c_str());
        }
    }

    log_main(parse_command);
}

void parse_command(const string &command)
{
    string word;
    if (command.find_first_of(' ') == string::npos)
    {
        word = command.substr(1);
    }
    else
    {
        word = command.substr(1, command.find_first_of(' ') - 2);
    }
    if (word == "start")
    {
    }
    else if (word == "play")
    {
    }
    else if (word == "add")
    {
    }
    else if (word == "skip")
    {
        session->skip_to_next(access_token);
    }
    else if (word == "back")
    {
        session->skip_to_previous(access_token);
    }
    else if (word == "remove")
    {
    }
}