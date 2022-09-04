#include "spotify-api.hpp"

namespace json = nlohmann;

template <typename... ParamTypes>
void setTimeout(int seconds, std::function<void(ParamTypes...)> callback, ParamTypes... params)
{
    // Even though we don't need a return value, the compiler will return an error
    // if we don't assign the return value of the async call (an std::future class) to a variable.
    // See https://riptutorial.com/cplusplus/example/4745/using-std--async-instead-of-std--thread for further info
    auto f = std::async(std::launch::async, [=]()
    {
        sleep(seconds);
        callback(params...);
    });
}

namespace spotify_api
{

    Api_Session * start_spotify_session(std::string &auth_code, const std::string &redirect_uri, std::string &client_keys_base64) {
        std::string form_data = "grant_type=authorization_code"
            "&code=" + http::url_encode(auth_code) +
            "&redirect_uri=" + http::url_encode(redirect_uri);;
        
        http::api_response response_data = http::post("https://accounts.spotify.com/api/token", form_data, client_keys_base64, false);

        api_token_response response_object;
        try
        {
            json::json response_json = json::json::parse(response_data.body);
            response_object.access_token = response_json["access_token"];
            response_object.token_type = response_json["token_type"];
            response_object.expires_in = response_json["expires_in"];
            response_object.refresh_token = response_json["refresh_token"];

        }
        catch(const std::exception& e)
        {
            fprintf(stderr, "spotify-api.cpp: get_auth_token(): Error: Failed to parse response data from Spotify.\nApi Response: %s.\nNow exiting.\n", response_data.body);
            exit(1);
        }
        Api_Session *new_session = new Api_Session(response_object);
        new_session->set_base64_id_secret(client_keys_base64);
        return new_session;
    }

    Api_Session::Api_Session(api_token_response &token_obj)
    {
        this->access_token = token_obj.access_token;
        this->refresh_token = token_obj.refresh_token;
        this->token_grant_time = time(NULL);
        this->token_expiration_time = this->token_grant_time + token_obj.expires_in;

        // C++ doesn't like you passing class member functions to other functions, and the template function setTimeout doesn't like std::bind.
        // I wish I could make this cleaner, but I'm too lazy to do that and I also don't want to inline this either.
        setTimeout<>(token_obj.expires_in - 60, (std::function<void()>) std::bind(&Api_Session::refresh_access_token, this));
    }

    void Api_Session::refresh_access_token()
    {
        std::string form_data = "grant_type=refresh_token"
            "&refresh_token=" + this->refresh_token;
        
        http::api_response response_data = http::post("https://accounts.spotify.com/api/token", form_data, this->base64_client_id_secret, false);

        json::json response_json = json::json::parse(response_data.body);
        try
        {
            this->access_token = response_json["access_token"];
            this->token_grant_time = time(NULL);
            this->token_expiration_time = this->token_grant_time + response_json["expires_in"].get<unsigned int>();
            if (response_json.contains("refresh_token")) {
                this->refresh_token = response_json["refresh_token"];
            }
        }
        catch(const std::exception& e)
        {
            fprintf(stderr, "endpoints.cpp: refresh_access_token(): Error: Failed to parse response data from Spotify.\nApi Response: %s.\nTrying again in 60s\n", response_data.body);
            setTimeout<>(60, (std::function<void()>) std::bind(&Api_Session::refresh_access_token, this));
            return;
        }
        setTimeout<>(response_json["expires_in"].get<unsigned int>() - 60, (std::function<void()>) std::bind(&Api_Session::refresh_access_token, this));
    }

    void Api_Session::set_base64_id_secret(string &new_value) {
        this->base64_client_id_secret = new_value;
    }

    // ALBUMS ----------------------------------------------------------------------------------------------------------------------------------------

    #define API_PREFIX "https://api.spotify.com/v1"

    album * Api_Session::get_album(const string &album_id) {
        album * retval = new album;
        string url = API_PREFIX "/albums/";
        url += album_id;

        http::api_response response = http::get(url.c_str(), string(""), this->access_token);
        if (response.code == 200) {
            object_from_json(response.body, *retval);
        }
        return retval;
    }

    std::vector<album> Api_Session::get_albums(const std::vector<string> &album_ids)
    {
        std::vector<album> albums;
        albums.resize(album_ids.size());
        string query_string = "";
        size_t num_batches = ceil(album_ids.size() / 20);

        for (size_t batch = 0; batch < num_batches; batch++)
        {
            size_t batch_size = batch * 20 + 20 < album_ids.size() ? 20 : album_ids.size() - (batch * 20 + 20);
            query_string = "ids=";
            for (size_t i = batch * 20; i < (batch * 20) + batch_size; i++) {
                query_string += album_ids.at(i) + ",";
            }
            query_string = std::regex_replace(query_string, std::regex(",+$"), "");
            http::api_response batch_response = http::get(API_PREFIX "/albums", query_string, this->access_token);
            if (batch_response.code == 200) {
                json::json albums_json = json::json::parse(batch_response.body)["albums"];
                for (auto new_album = albums_json.begin(); new_album != albums_json.end(); ++new_album) {
                    album temp;
                    object_from_json(new_album.value().dump(), temp);
                    albums.push_back(temp);
                }
            }
        }
        return albums;
    }

    track * Api_Session::get_currently_playing_track()
    {
        track * retval = new track;
        http::api_response response = http::get(api_prefix "/me/player/currently-playing", std::string(""), access_token);
        printf("current track res code: %u\n", response.code);
        if (response.code == 200) {
            object_from_json(response.body, *retval);
        }
        return retval;
    }

    track * Api_Session::search_for_track(const string &search_query) {
        track * retval = new track;
        string query_string = "q=" + http::url_encode(search_query) + "&type=track&limit=1";
        http::api_response res = http::get(api_prefix "/search", query_string, access_token);

        if (res.code == 200) {
            json::json json_res = json::json::parse(res.body);
            object_from_json(json_res["tracks"]["items"][0].dump(), *retval);
        }
        return retval;
    }

    void object_from_json(const std::string &json_string, album &output)
    {
        // printf("album:\n%s", json_string.c_str());
        try
        {
            json::json json_object = json::json::parse(json_string);

            output.album_type = json_object["album_type"];
            
            json::json temp = json_object["artists"];
            output.artists.reserve(temp.size());
            for (auto artist = temp.begin(); artist != temp.end(); ++artist)
            {
                spotify_api::artist *temp_artist = new spotify_api::artist;
                object_from_json(artist.value().dump(), *temp_artist);
                output.artists.push_back(temp_artist);
            }
            
            temp = json_object["available_markets"];
            output.available_markets.reserve(temp.size());
            for (auto market = temp.begin(); market != temp.end(); ++market)
            {
                output.available_markets.push_back(market.value());
            }

            temp = json_object["external_urls"];
            for (auto ext_url = temp.begin(); ext_url != temp.end(); ++ext_url)
            {
                output.external_urls.insert(std::pair<string, string>(ext_url.key(), ext_url.value().get<string>()));
            }

            output.href = json_object["href"];

            output.id = json_object["id"];

            temp = json_object["images"];
            output.images.reserve(temp.size());
            for (auto image = temp.begin(); image != temp.end(); ++image)
            {
                image_t temp_image;
                temp_image.url = image.value()["url"];
                temp_image.width = image.value()["width"];
                temp_image.height = image.value()["height"];
                output.images.push_back(temp_image);
            }

            output.name = json_object["name"];

            output.release_date = json_object["release_date"];

            output.release_date_precision = json_object["release_date_precision"];

            output.total_tracks = json_object["total_tracks"];

            output.uri = json_object["uri"];

            if (json_object.contains("tracks")) {
                json::json json_tracks = json_object["tracks"];
                album_tracks temp_track;
                temp_track.href = json_tracks["href"];
                temp_track.limit = json_tracks["limit"];
                temp_track.offset = json_tracks["offset"];
                output.tracks.total = json_tracks["total"];
                if (!json_tracks["next"].is_null()) output.tracks.next = json_tracks["next"];
                if (!json_tracks["previous"].is_null()) output.tracks.previous = json_tracks["previous"];
                json_tracks = json_tracks["items"];
                output.tracks.items.reserve(json_tracks.size());
                for (auto track = json_tracks.begin(); track != json_tracks.end(); ++track) {
                    spotify_api::track *temp = new spotify_api::track;
                    object_from_json(track.value().dump(), *temp);
                    output.tracks.items.push_back(temp);
                }
                
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            fprintf(stderr, "\nspotify-api/endpoints.cpp: object_from_json(): Error: Failed to convert json data to album.\nJson string: %s\nNow exiting.\n", json_string.c_str());
            exit(1);
        }
        
    }

    void object_from_json(const std::string &json_string, artist &output)
    {
        try
        {
            /* TODO: maybe replace with a custom function to parse an array.
             * Array parser can take the output location and array data as arguments
             * as well as a callback function to run on each array element.
             * So in theory this would work similar to the map() or forEach() array method in JS.
             */ 
            json::json json_object = json::json::parse(json_string);
            auto url_obj = json_object["external_urls"];
            for (auto url = url_obj.begin(); url != url_obj.end(); ++url) {
                output.external_urls.insert(std::pair<std::string, std::string>(url.key(), url.value()));
            }

            if (json_object.contains("followers")) {
                output.followers.href = json_object["followers"]["href"];
                output.followers.total = json_object["followers"]["total"];
            }
            
            if (json_object.contains("genres")) {
                auto genre_array = json_object["genres"];
                output.genres.reserve(genre_array.size());
                for (auto genre = genre_array.begin(); genre != genre_array.end(); ++genre) {
                    output.genres.push_back(genre.value());
                }
            }

            output.href = json_object["href"];
            output.id = json_object["id"];
            
            if (json_object.contains("images")) {
                auto image_array = json_object["images"];
                output.images.reserve(image_array.size());
                for (auto image = image_array.begin(); image != image_array.end(); ++image) {
                    image_t temp;
                    temp.url = image.value()["url"];
                    temp.width = image.value()["width"];
                    temp.height = image.value()["height"];
                    output.images.push_back(temp);
                }
            }

            output.name = json_object["name"];
            if (json_object.contains("popularity")) output.popularity = json_object["popularity"];
            output.uri = json_object["uri"];
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            fprintf(stderr, "\nspotify-api/artist.cpp: object_from_json(): Error: Failed to convert json data to artist.\nJson string: %s\nNow exiting.\n", json_string.c_str());
            exit(1);
        }
    }

    void object_from_json(const std::string &json_string, track &output)
    {

        int step = 1;
        try
        {
            json::json json_object = json::json::parse(json_string);
            if (json_object.contains("item")) {
                json_object = json_object["item"];
            }
            
            if (json_object.contains("album")) {
                spotify_api::album *temp = new spotify_api::album;
                object_from_json(json_object["album"].dump(), *temp);
                output.album = temp;
            }
            
            json::json temp_obj = json_object["artists"];
            output.artists.reserve(temp_obj.size());
            for (auto artist = temp_obj.begin(); artist != temp_obj.end(); ++artist) {
                spotify_api::artist *temp = new spotify_api::artist;
                object_from_json(artist.value().dump(), *temp);
                output.artists.push_back(temp);
            }

            step++;

            temp_obj = json_object["available_markets"];
            output.available_markets.reserve(temp_obj.size());
            for (auto market = temp_obj.begin(); market != temp_obj.end(); ++market) {
                output.available_markets.push_back(market.value());
            }

            step++;

            output.disc_number = json_object["disc_number"];
            step++;
            output.duration_ms = json_object["duration_ms"];
            step++;

            if (json_object.contains("external_ids")) {
                temp_obj = json_object["external_ids"];
                for (auto ext_id = temp_obj.begin(); ext_id != temp_obj.end(); ++ext_id) {
                    output.external_ids.insert(std::pair<std::string, std::string>(ext_id.key(), ext_id.value()));
                }
            }

            step++;

            temp_obj = json_object["external_urls"];
            for (auto ext_url = temp_obj.begin(); ext_url != temp_obj.end(); ++ext_url) {
                output.external_urls.insert(std::pair<std::string, std::string>(ext_url.key(), ext_url.value()));
            }

            step++;

            output.href = json_object["href"];
            step++;
            output.id = json_object["id"];
            step++;
            output.is_explicit = json_object["explicit"];
            step++;
            output.is_local = json_object["is_local"];
            step++;
            if (json_object.contains("is_playable")) output.is_playable = json_object["is_playable"];
            
            step++;

            if (json_object.contains("linked_from")) {
                track *linked_track; // We use a pointer here so there wont be a dangling pointer when the function returns
                object_from_json(json_object["linked_from"], *linked_track); // Could recurse infinitely and cause stack overflow. Maybe look into this.
                output.linked_from.emplace(linked_track);
            } else {
                output.linked_from.reset();
            }

            step++;

            output.name = json_object["name"];
            step++;
            if (json_object.contains("popularity")) output.popularity = json_object["popularity"];
            step++;
            if (json_object.contains("preview_url")) output.preview_url = json_object["preview_url"];
            step++;

            if (json_object.contains("restrictions")) {
                temp_obj = json_object["restrictions"];
                for (auto restriction = temp_obj.begin(); restriction != temp_obj.end(); ++restriction) {
                    output.restrictions.insert(std::pair<std::string, std::string>(restriction.key(), restriction.value()));
                }
            }

            step++;

            output.track_number = json_object["track_number"];
            step++;
            output.uri = json_object["uri"];
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            fprintf(stderr, "\nspotify-api/endpoints.cpp: object_from_json(): Error: Failed to convert json data to track at step %u.\nJson string: %s\nNow exiting.\n", step, json_string.c_str());
            exit(1);
        }
        
    }

} // namespace spotify_api
