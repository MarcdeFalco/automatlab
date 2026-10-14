# Automatlab

Prototype d'un laboratoire d'expressions régulières et d'automates, écrit en C
avec [raylib](https://www.raylib.com/) et [raygui](https://github.com/raysan5/raygui).

## Ce que fait l'outil (état actuel)

- **Saisie d'une expression régulière** dans la zone de texte en haut à gauche
  (cliquer dans la zone, taper, valider par Entrée).
- **Analyse syntaxique** par descente récursive de la grammaire

      regexp  = concat '|' regexp | concat
      concat  = facteur concat | facteur
      facteur = base '*' | base
      base    = lettre | '(' regexp ')'

  Les lettres sont les caractères alphanumériques ; la lettre `e` désigne le
  mot vide ε (elle ne peut donc pas servir de lettre). Pas de symbole pour ∅,
  pas de `+` ni de `?`, pas d'espaces. En cas d'erreur, « Parsing error »
  s'affiche.
- **Construction de Thompson** de l'automate avec ε-transitions (un état
  initial, un état final).
- **Affichage** de l'automate avec un placement par forces (Fruchterman-Reingold) :
  état initial en rouge, état final cerclé de vert, transitions étiquetées
  par leur lettre (`e` pour une ε-transition).

Ne sont **pas** (encore) implémentés : Glushkov, suppression des
ε-transitions, déterminisation, minimisation, lecture d'un mot. Les
transitions sont dessinées sans flèche.

## Compilation native

Il faut raylib (testé avec raylib 5.5 ; `brew install raylib` sous macOS).

    make          # produit ./main
    make run
    make test     # compare les automates de Thompson à regex.h sur les mots de longueur ≤ 7

Pour un raylib installé ailleurs :
`make RAYLIB_INC=/chemin/include RAYLIB_LIB=/chemin/lib`.

## Compilation web (emscripten)

Il faut [emsdk](https://emscripten.org/docs/getting_started/downloads.html)
(`emcc` dans le `PATH`) et une `libraylib.a` compilée pour le web :

    git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib
    (cd raylib/src && make PLATFORM=PLATFORM_WEB)
    make web RAYLIB_WEB_SRC=$PWD/raylib/src

On obtient `web/index.html`, `web/index.js` et `web/index.wasm`, à servir par
HTTP (`make runserver` puis http://localhost:1234) ou à publier tel quel
(GitHub Pages).

## Licence

raygui (`raygui.h`) et raylib sont sous licence zlib/libpng.
