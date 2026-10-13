#include "core/MovieCatalog.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "api/tsvitch/util/http.hpp"

tsvitch::LiveM3u8 Movie::toChannel() const {
    tsvitch::LiveM3u8 c;
    c.id         = id;
    c.chno       = "";
    c.title      = title;
    c.logo       = cover;
    c.groupTitle = genre;
    c.url        = url;
    return c;
}

static std::string str(const nlohmann::json& j, const char* key) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    if (j.contains(key) && j[key].is_number()) return std::to_string(j[key].get<int>());
    return "";
}

bool MovieCatalog::fetch(const std::string& url, std::vector<Movie>& out, std::string& error) {
    cpr::Response r = tsvitch::HTTP::get(url);
    if (r.error || r.status_code != 200) {
        error = r.error ? r.error.message : ("HTTP " + std::to_string(r.status_code));
        return false;
    }

    try {
        auto root = nlohmann::json::parse(r.text);
        // aceita {"filmes":[...]} ou direto [...]
        const nlohmann::json* list = &root;
        if (root.is_object()) {
            if (root.contains("filmes"))      list = &root["filmes"];
            else if (root.contains("movies")) list = &root["movies"];
        }
        if (!list->is_array()) {
            error = "catalogo.json invalido: esperado uma lista de filmes";
            return false;
        }

        size_t i = 0;
        for (const auto& j : *list) {
            Movie m;
            m.title       = str(j, "titulo");
            if (m.title.empty()) m.title = str(j, "title");
            m.url         = str(j, "url");
            if (m.title.empty() || m.url.empty()) continue;  // ignora entradas incompletas

            m.id          = str(j, "id");
            if (m.id.empty()) m.id = std::to_string(i);
            m.description = str(j, "descricao");
            if (m.description.empty()) m.description = str(j, "description");
            m.cover       = str(j, "capa");
            if (m.cover.empty()) m.cover = str(j, "cover");
            m.backdrop    = str(j, "banner");
            if (m.backdrop.empty()) m.backdrop = m.cover;
            m.addedAt     = str(j, "adicionado_em");
            m.year        = str(j, "ano");
            m.genre       = str(j, "genero");
            out.push_back(std::move(m));
            i++;
        }
    } catch (const std::exception& e) {
        error = std::string("Erro lendo catalogo.json: ") + e.what();
        return false;
    }
    return true;
}

void MovieCatalog::sortNewestFirst(std::vector<Movie>& movies) {
    // Sem data: mantem a ordem do arquivo, considerando o fim da lista como o mais novo.
    std::vector<size_t> order(movies.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        if (movies[a].addedAt != movies[b].addedAt) return movies[a].addedAt > movies[b].addedAt;
        return a > b;
    });
    std::vector<Movie> sorted;
    sorted.reserve(movies.size());
    for (auto i : order) sorted.push_back(std::move(movies[i]));
    movies = std::move(sorted);
}
