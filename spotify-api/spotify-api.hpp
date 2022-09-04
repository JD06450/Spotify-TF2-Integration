#ifndef _SPOTIFY_API_FILE_
#define _SPOTIFY_API_FILE_

#include <iostream>
#include <optional>
#include <unistd.h>
#include <string>
#include <future>
#include <vector>
#include <regex>
#include <ctime>
#include <map>

#include <nlohmann/json.hpp>
#include "../curl-util.hpp"

using std::string;

namespace json = nlohmann;

namespace spotify_api
{
    struct follower_t {
        std::string href;
        int total;
    };
    struct image_t {
        std::string url;
        int width;
        int height;
    };
    struct api_token_response {
        std::string access_token;
        std::string token_type;
        int expires_in;
        std::string refresh_token;
    };

    struct album;
    struct artist;
    struct track;
    struct album_tracks {
        string href;
        std::vector<track *> items;
        int limit;
        string next;
        int offset;
        string previous;
        int total;
    };

    struct album
    {
        string album_type;
        int total_tracks;
        std::vector<string> available_markets;
        std::map<string, string> external_urls;
        string href;
        string id;
        std::vector<image_t> images;
        string name;
        string release_date;
        string release_date_precision;
        std::map<string, string> restrictions;
        const string type = "album";
        string uri;
        std::vector<artist *> artists;
        album_tracks tracks;
    };
    

    struct artist
    {
        std::map<string, string> external_urls;
        follower_t followers;
        std::vector<string> genres;
        string href;
        string id;
        std::vector<image_t> images;
        string name;
        int popularity;
        const string type = "artist";
        string uri;
    };

    struct track
    {
        struct album *album;
        std::vector<struct artist *> artists;
        std::vector<string> available_markets;
        int disc_number;
        int duration_ms;
        bool is_explicit;
        std::map<string, string> external_ids;
        std::map<string, string> external_urls;
        string href;
        string id;
        bool is_playable;
        std::optional<track *> linked_from;
        std::map<string, string> restrictions;
        string name;
        int popularity;
        string preview_url;
        int track_number;
        const string type = "track";
        string uri;
        bool is_local;
    };

    void object_from_json(const string &json_string, album &output);
    void object_from_json(const string &json_string, artist &output);
    void object_from_json(const string &json_string, track &output);

    #define api_prefix "https://api.spotify.com/v1"

    class Api_Session {
        private:
            string access_token;
            string refresh_token;
            string base64_client_id_secret;
            unsigned long int token_grant_time;
            unsigned long int token_expiration_time;

        public:
            Api_Session(api_token_response &token_obj);
            Api_Session(string json_string);

            void refresh_access_token();
            void set_base64_id_secret(string &new_value);
            
            // ALBUMS --------------------------------------------------------------------------------------------------------------------------------

            album * get_album(const string &album_id);
            std::vector<album> get_albums(const std::vector<string> &album_ids);

            // PLAYER --------------------------------------------------------------------------------------------------------------------------------

            /*
            * Usage: retrieves the user's current playback state
            * Endpoint: /me/player
            * Parameters: none
            * Documentation: https://developer.spotify.com/documentation/web-api/reference/#/operations/get-information-about-the-users-current-playback
            */
            http::api_response inline get_playback_state(const std::string &access_token)
            {
                return http::get(api_prefix "/me/player", std::string(""), access_token);
            }

            /*
            * ***WIP***
            * Usage: Transfers media playback to another device
            * Endpoint: /me/player
            * Parameters:
            * --  device_ids: string array: An array containing the is of the device where playback should be transferred to
            * --  play: boolean: If true, then playback is ensured on the new device, otherwise the old state is used
            * Documentation: https://developer.spotify.com/documentation/web-api/reference/#/operations/transfer-a-users-playback
            */
            http::api_response inline transfer_playback(const std::string &access_token);

            /*
            * Usage: Get information on the user's available devices
            * Endpoint: /me/player/devices
            * Parameters: none
            * Documentation: https://developer.spotify.com/documentation/web-api/reference/#/operations/get-a-users-available-devices
            */
            http::api_response inline get_available_devices(const std::string &access_token)
            {
                return http::get(api_prefix "/me/player/devices", std::string(""), access_token);
            }

            
            
            bool inline skip_to_previous(const string &access_token) {
                http::api_response response = http::post(api_prefix "/me/player/previous", string(""), access_token, true);
                if (response.code == 204) {
                    return true;
                } else {
                    return false;
                }
            }

            bool inline skip_to_next(const string &access_token) {
                http::api_response response = http::post(api_prefix "/me/player/next", string(""), access_token, true);
                if (response.code == 204) {
                    return true;
                } else {
                    return false;
                }
            }

            // PLAYLISTS -----------------------------------------------------------------------------------------------------------------------------

            void get_my_playlists();

            // TRACKS --------------------------------------------------------------------------------------------------------------------------------

            /*
            * Usage: Get the track the the user is currently playing
            * Endpoint: /me/player/currently-playing
            * Parameters: none
            * Documentation: https://developer.spotify.com/documentation/web-api/reference/#/operations/get-the-users-currently-playing-track
            */
            track * get_currently_playing_track();
            track * search_for_track(const string &search_query);
    };

    // Authentication Process Step 2
    // Using the authentication code retrieved from the Spotify Web API, we send another request to generate an access token.
    Api_Session * start_spotify_session(std::string &auth_code, const std::string &redirect_uri, std::string &client_keys_base64);
} // namespace spotify_api

#endif