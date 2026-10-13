# TsVitch -> Catalogo de filmes estilo Netflix

## O que mudou
- NOVO  tsvitch/source/fragment/netflix_home.cpp (+ .hpp): tela inicial estilo Netflix
- NOVO  tsvitch/source/core/MovieCatalog.cpp (+ .hpp): le o catalogo.json
- NOVO  tsvitch/include/config/catalog_config.h: **troque CATALOG_URL pelo link do seu catalogo.json**
- EDIT  resources/xml/activity/main.xml: aba Home agora usa <NetflixHome/>
- EDIT  tsvitch/source/utils/register_helper.cpp: registra a NetflixHome
- NOVO  catalogo.json: modelo para voce preencher

## Como usar
1. Suba o catalogo.json no GitHub (ou outro lugar) e copie o link "raw".
2. Cole o link em catalog_config.h (CATALOG_URL).
3. Compile pelo GitHub Actions (.github/workflows/build.yaml) como no TsVitch original.

## catalogo.json
Campos: titulo, url (obrigatorios); capa, banner, descricao, adicionado_em (AAAA-MM-DD), ano, genero.
- Os 5 mais novos (pela data adicionado_em) vao em "Novos adicionados".
- Sem data: o ultimo da lista conta como o mais novo.

## Controles
- D-pad: navega; o banner muda conforme o poster selecionado
- A: assistir | X: recarregar catalogo
