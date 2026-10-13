#pragma once

#include <string>
#include <vector>
#include <functional>

#include "api/tsvitch/result/home_live_result.h"

struct Movie {
    std::string id;
    std::string title;
    std::string description;
    std::string cover;     // capa vertical (poster)
    std::string backdrop;  // imagem larga do banner (opcional, usa cover se vazio)
    std::string url;       // link direto do video (.mp4/.mkv)
    std::string addedAt;   // "AAAA-MM-DD" (ordena os mais novos)
    std::string year;
    std::string genre;

    // Converte para o tipo que o player do TsVitch ja entende.
    tsvitch::LiveM3u8 toChannel() const;
};

class MovieCatalog {
public:
    // Baixa e le o catalogo (chamar fora da thread de UI).
    static bool fetch(const std::string& url, std::vector<Movie>& out, std::string& error);

    // Ordena do mais novo para o mais antigo.
    static void sortNewestFirst(std::vector<Movie>& movies);
};
