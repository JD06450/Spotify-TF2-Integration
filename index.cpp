#include "index.hpp"

#include <spotify-api.hpp>
#include "base64.hpp"
#include "tf2-logfile.hpp"

using std::string;
using std::fstream;
using std::cout, std::endl;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpResponse = std::shared_ptr<HttpServer::Response>;
using HttpRequest = std::shared_ptr<HttpServer::Request>;

void parse_command(const string &command);

string access_token;

spotify_api::Spotify_API *session;

std::map<string, string> * process_env_vars(fstream &file)
{
    //TODO: process env vars for output file, console file, and username

    std::map<string, string> * env_vars = new std::map<string, string>();

    char name_buffer[256];
    char value_buffer[256];

    while (!file.eof())
    {
        if (file.bad())
        {
            printf("File operation failed");
            return env_vars;
        }
        file.getline(name_buffer, 256, '=');

        file.getline(value_buffer, 256);

        std::regex_search(name_buffer, std::regex("[]*"));

        if (!strcmp(name_buffer, "CLIENT_ID"))
        {
            if (std::regex_search(value_buffer, std::regex("[^A-Fa-f0-9]")) || strnlen(value_buffer, 50) > 32)
            {
                printf("Environment variable invalid: %s\n", name_buffer);
            }

            env_vars->insert(std::pair<string, string>("CLIENT_ID", value_buffer));
        }
        else if (!strcmp(name_buffer, "CLIENT_SECRET"))
        {
            if (std::regex_search(value_buffer, std::regex("[^A-Fa-f0-9]")) || strnlen(value_buffer, 50) > 32)
            {
                printf("Environment variable invalid: %s\n", name_buffer);
            }
            env_vars->insert(std::pair<string, string>("CLIENT_SECRET", value_buffer));
        }
    }

    return env_vars;
}

int main()
{
    fstream env_file_stream;
    int env_vars = 2;
    string client_id;
    string client_secret;
    char auth_token[64];
    

    env_file_stream.open(".env", fstream::in);



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

    server.on_error = [](std::shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/)
    {
        // Handle errors here
        // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
    };

    std::promise<unsigned short> server_port;
    std::thread *server_thread = new std::thread([&server, &server_port]()
    {
        server.start([&server_port](unsigned short port)
        {
            server_port.set_value(port);
        });
    });

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
    delete server_thread;

    cout << "Simple Web Server destroyed." << endl;

    session = new spotify_api::Spotify_API(auth_code, "http://localhost:8080", auth_base64);
    cout << "Session Created" << endl;

    spotify_api::track_t *current_track = session->player_api->get_currently_playing_track();

    printf("Currently Playing: %s by %s\nFrom album: %s\n", current_track->name.c_str(), current_track->artists[0]->name.c_str(), current_track->album->name.c_str());

    spotify_api::album_full_t *song_album = session->album_api->get_album(current_track->album->id);

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

    std::vector<spotify_api::playlist_t *> my_playlists = session->playlist_api->get_my_playlists();
    puts("\nMy playlists:");
    for (int i = 0; i < my_playlists.size(); i++)
    {
        fprintf(stdout, "%02i: %s\n", i, my_playlists[i]->name.c_str());
    }

    // replace with while loop here

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
    else if (word == "add")
    {
    }
    else if (word == "skip")
    {
        session->player_api->skip_to_next(access_token);
    }
    else if (word == "prev")
    {
        session->player_api->skip_to_previous(access_token);
    }
}