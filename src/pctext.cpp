// The game's texts about the PlayStation 2 (its memory cards and slots, controller ports, the console's power) said as a
// computer's, as the Crash Twinsanity native port does (its native/PC_TEXT.md): the lines of WoC's text table (0x5E0590, a
// line's text for each of six languages) that name the console are pointed at new lines. Each slot's original is checked
// first: if any differs (another executable) nothing is changed. The new lines are written over the old lines' own bytes,
// which nothing else uses (every one of them only appears in these messages). Every original and replacement is in
// PC_TEXT.md. Lines with %i keep it where the original had it: the game prints the free space it needs there.
//
// The font's own codes for the accented capitals: 0x8E for Ä, 0x99 Ö, 0x9A Ü, 0xA0 á.
#include "ps2/iop.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
struct Line
{
    u32 slot;            // the text table's pointer to the line
    const char* original;
    const char* text;    // nullptr: kept
};

const Line Lines[] = {
    {0x005e14f0, "INSERT CONTROLLER IN", "PLEASE CONNECT"},
    {0x005e14f4, "CONTROLLER PORT 1", "A CONTROLLER"},
    {0x005e14f8, "INSERER UNE MANETTE DANS", "VEUILLEZ BRANCHER"},
    {0x005e14fc, "LE PORT DE MANETTE 1", "UNE MANETTE"},
    {0x005e1500, "CONTROLLER IN CONTROLLER-", "BITTE EINEN CONTROLLER"},
    {0x005e1504, "ANSCHLUSS EINSTECKEN", "ANSCHLIESSEN"},
    {0x005e1508, "INSERISCI CONTROLLER ", "COLLEGA UN"},
    {0x005e150c, "NELL'INGRESSO CONTROLLER", "CONTROLLER"},
    {0x005e1510, "INSERTA EL CONTROLADOR", "CONECTA UN"},
    {0x005e1514, "EN EL PUERTO 1", "CONTROLADOR"},
    {0x005e1518, "SLUIT CONTROLLER AAN", "SLUIT EEN"},
    {0x005e151c, "OP CONTROLLER-AANSLUITING", "CONTROLLER AAN"},
    {0x005e1520, "CHECKING MEMORY CARD", "CHECKING SAVE DATA"},
    {0x005e1524, "(8MB) (FOR p)", ""},
    {0x005e1528, "VERIFICATION DE LA MEMORY", "VERIFICATION DES"},
    {0x005e152c, "CARD (8MO) (p)", "SAUVEGARDES"},
    {0x005e1530, "\x9a""BERPR\x9a""FE MEMORY CARD", "SPIELST\x8e""NDE WERDEN"},
    {0x005e1534, "(8MB) (p)...", "\x9a""BERPR\x9a""FT"},
    {0x005e1538, "CONTROLLO MEMORY CARD (8MB)", "CONTROLLO DEI"},
    {0x005e153c, "(p) IN CORSO", "SALVATAGGI IN CORSO"},
    {0x005e1540, "COMPROBANDO TARJETA DE", "COMPROBANDO"},
    {0x005e1544, "MEMORIA (8MB) (p)", "DATOS GUARDADOS"},
    {0x005e1548, "GEHEUGENKAART WORDT GECONTROLEERD", "OPGESLAGEN GEGEVENS"},
    {0x005e154c, "(8MB) (p)", "WORDEN GECONTROLEERD"},
    {0x005e15f8, "PLEASE INSERT MEMORY CARD", "SAVE DATA"},
    {0x005e15fc, "(8MB) (FOR p)", "IS NOT AVAILABLE"},
    {0x005e1600, "IN MEMORY CARD SLOT 1", ""},
    {0x005e1604, "INSERER UNE MEMORY CARD", "SAUVEGARDES"},
    {0x005e1608, "(8MO) (p)", "INDISPONIBLES"},
    {0x005e160c, "DANS LA FENTE N\xf8""1", ""},
    {0x005e1610, "BITTE MEMORY CARD", "SPEICHERN IST"},
    {0x005e1614, "(8MB) (p) IN MEMORY", "NICHT M\x99""GLICH"},
    {0x005e1618, "CARD-STECKPLATZ 1 EINLEGEN", ""},
    {0x005e161c, "INSERISCI LA MEMORY CARD", "SALVATAGGI"},
    {0x005e1620, "(8MB) (p)", "NON DISPONIBILI"},
    {0x005e1624, "NELL'INGRESSO MEMORY CARD 1", ""},
    {0x005e1628, "INSERTE LA TARJETA DE MEMORIA", "LOS DATOS GUARDADOS"},
    {0x005e162c, "(8MB) (p)", "NO EST\xa0""N DISPONIBLES"},
    {0x005e1630, "EN LA RANURA 1", ""},
    {0x005e1634, "VOER GEHEUGENKAART", "OPSLAAN IS"},
    {0x005e1638, "(8MB) (p)", "NIET MOGELIJK"},
    {0x005e163c, "IN AANSLUITING 1 IN", ""},
    {0x005e1898, "MEMORY CARD", "THE SAVE DATA"},
    {0x005e189c, "(8MB) (FOR p)", "COULD NOT BE READ"},
    {0x005e18a0, "IS NOT FORMATTED", ""},
    {0x005e18a4, "LA MEMORY CARD (8MO) (p)", "LES SAUVEGARDES"},
    {0x005e18a8, "INSEREE DANS LA FENTE N\xf8""1", "SONT ILLISIBLES"},
    {0x005e18ac, "N'EST PAS FORMATEE", ""},
    {0x005e18b0, "MEMORY CARD (8MB) (p)", "DIE SPIELST\x8e""NDE"},
    {0x005e18b4, "IN STECKPLATZ 1", "SIND NICHT LESBAR"},
    {0x005e18b8, "IST NICHT FORMATIERT", ""},
    {0x005e18bc, "LA MEMORY CARD (8MB) (p)", "IMPOSSIBILE LEGGERE"},
    {0x005e18c0, "NELL'INGRESSO MEMORY CARD 1", "I SALVATAGGI"},
    {0x005e18c4, "NON \x8a"" FORMATTATA", ""},
    {0x005e18c8, "LA TARJETA DE MEMORIA", "NO SE PUEDEN LEER"},
    {0x005e18cc, "(8MB) (p) DE LA", "LOS DATOS GUARDADOS"},
    {0x005e18d0, "RANURA 1 NO EST\xa0""FORMATEADA", ""},
    {0x005e18d4, "DE GEHEUGENKAART (8MB)", "DE OPGESLAGEN GEGEVENS"},
    {0x005e18d8, "(p) IN GEHEUGENKAART-", "ZIJN NIET LEESBAAR"},
    {0x005e18dc, "AANSLUITING 1 IS NIET GEFORMATTEERD", ""},
    {0x005e18e0, "NOT ENOUGH SPACE ON", "NOT ENOUGH SPACE"},
    {0x005e18e4, "MEMORY CARD", "ON THE COMPUTER"},
    {0x005e18e8, "(8MB) (FOR p)", ""},
    {0x005e18ec, "", ""},
    {0x005e18f0, "ESPACE INSUFFISANT SUR LA MEMORY", "ESPACE INSUFFISANT"},
    {0x005e18f4, "CARD (8MO) (p) POUR", "SUR L'ORDINATEUR POUR"},
    {0x005e18f8, "SAUVEGARDER LA PARTIE", nullptr},
    {0x005e18fc, "", ""},
    {0x005e1900, "NICHT GEN\x9a""GEND PLATZ AUF MEMORY", "NICHT GEN\x9a""GEND PLATZ"},
    {0x005e1904, "CARD (8MB) (p) ZUM", "AUF DEM COMPUTER ZUM"},
    {0x005e1908, "SPEICHERN DES SPIELS", nullptr},
    {0x005e190c, "", ""},
    {0x005e1910, "SPAZIO INSUFFICIENTE SULLA MEMORY", "SPAZIO INSUFFICIENTE"},
    {0x005e1914, "CARD (8MB) (p) PER", "SUL COMPUTER PER"},
    {0x005e1918, "SALVARE LA PARTITA", nullptr},
    {0x005e191c, "", ""},
    {0x005e1920, "NO HAY ESPACIO SUFICIENTE EN LA", "NO HAY ESPACIO SUFICIENTE"},
    {0x005e1924, "TARJETA DE MEMORIA (8MB) (p)", "EN EL ORDENADOR"},
    {0x005e1928, "PARA GRABAR LA PARTIDA", nullptr},
    {0x005e192c, "", ""},
    {0x005e1930, "ONVOLDOENDE RUIMTE OP DE", "ONVOLDOENDE RUIMTE"},
    {0x005e1934, "GEHEUGENKAART (8MB) (p)", "OP DE COMPUTER"},
    {0x005e1938, "OM SPEL OP TE SLAAN", nullptr},
    {0x005e193c, "", ""},
    {0x005e1970, "PLEASE DO NOT REMOVE MEMORY", "PLEASE DON'T TURN"},
    {0x005e1974, "CARD (8MB) (FOR p)", "OFF YOUR COMPUTER"},
    {0x005e1978, "OR SWITCH OFF POWER", ""},
    {0x005e197c, "NE PAS RETIRER LA MEMORY", "N'ETEIGNEZ PAS"},
    {0x005e1980, "CARD (8MO) (p)", "VOTRE ORDINATEUR"},
    {0x005e1984, "NI ETEINDRE LA CONSOLE", ""},
    {0x005e1988, "BITTE MEMORY CARD (8MB)", "BITTE DEN COMPUTER"},
    {0x005e198c, "(p) NICHT ENTFERNEN", "NICHT AUSSCHALTEN"},
    {0x005e1990, "UND STROM NICHT AUSSCHALTEN", ""},
    {0x005e1994, "NON RIMUOVERE MEMORY CARD", "NON SPEGNERE"},
    {0x005e1998, "(8MB) (p)", "IL COMPUTER"},
    {0x005e199c, "E NON SPEGNERE CONSOLE", ""},
    {0x005e19a0, "NO RETIRE LA TARJETA DE", "NO APAGUES"},
    {0x005e19a4, "MEMORIA (8MB) (p)", "EL ORDENADOR"},
    {0x005e19a8, "NI CORTE LA ALIMENTACI\xa2""N", ""},
    {0x005e19ac, "VERWIJDER GEHEUGENKAART", "SCHAKEL DE COMPUTER"},
    {0x005e19b0, "(8MB) (p) NIET EN", "NIET UIT"},
    {0x005e19b4, "SCHAKEL DE STROOM NIET UIT", ""},
    {0x005e19b8, "YOU NEED A MEMORY CARD", ""},
    {0x005e19bc, "(8MB) (FOR p)", "YOU NEED AT LEAST"},
    {0x005e19c0, "WITH AT LEAST %iKB OF FREE SPACE TO", "%iKB OF FREE SPACE TO"},
    {0x005e19c4, "SAVE YOUR PROGRESS", nullptr},
    {0x005e19c8, "PLEASE INSERT A MEMORY CARD", ""},
    {0x005e19cc, "(8MB) (FOR p)", "PLEASE FREE UP AT LEAST"},
    {0x005e19d0, "WITH AT LEAST %iKB OR YOU WILL", "%iKB OF SPACE OR YOU WILL"},
    {0x005e19d4, "BE UNABLE TO SAVE", nullptr},
    {0x005e19d8, "NECESSITE CARTE MEMOIRE", ""},
    {0x005e19dc, "(8MO) (p)", "IL VOUS FAUT AU MOINS"},
    {0x005e19e0, "AVEC AU MOINS %iKO D'ESPACE LIBRE", "%iKO D'ESPACE LIBRE"},
    {0x005e19e4, "POUR SAUVEGARDER LA PARTIE", nullptr},
    {0x005e19e8, "VEUILLEZ INSERER CARTE MEMOIRE", ""},
    {0x005e19ec, "(8MO) (p)", "VEUILLEZ LIBERER AU MOINS"},
    {0x005e19f0, "AVEC AU MOINS %iKO SINON IL SERA", "%iKO SINON IL SERA"},
    {0x005e19f4, "IMPOSSIBLE DE SAUVEG", nullptr},
    {0x005e19f8, "SIE BEN\x94""TIGEN EINE MEMORY CARD", ""},
    {0x005e19fc, "(8MB) (p)", "SIE BEN\x99""TIGEN MINDESTENS"},
    {0x005e1a00, "MIT MIND. %iKB FREIEM SPEICHER, UM IHRE", "%iKB FREIEN SPEICHER, UM IHRE"},
    {0x005e1a04, "SPIELDATEN ZU SPEICHERN", nullptr},
    {0x005e1a08, "SETZEN SIE DAHER F\x9a""R DEN SPEICHERVORGANG", ""},
    {0x005e1a0c, "EINE MEMORY CARD", "UM SPEICHERN ZU K\x99""NNEN,"},
    {0x005e1a10, "(8MB) (p)", "GEBEN SIE BITTE MINDESTENS"},
    {0x005e1a14, "MIT MIND. %iKB EIN", "%iKB SPEICHER FREI"},
    {0x005e1a18, "USA UNA MEMORY CARD", ""},
    {0x005e1a1c, "(8MB) (p)", "SERVE SPAZIO SUL COMPUTER:"},
    {0x005e1a20, "CON ALMENO %iKB DI SPAZIO LIBERO PER", "ALMENO %iKB LIBERI PER"},
    {0x005e1a24, "SALVARE PROGRESSI", nullptr},
    {0x005e1a28, "INSERISCI UNA MEMORY CARD", ""},
    {0x005e1a2c, "(8MB) (p)", "LIBERA SPAZIO SUL COMPUTER:"},
    {0x005e1a30, "CON ALMENO %iKB PER", "ALMENO %iKB PER"},
    {0x005e1a34, "POTER SALVARE", nullptr},
    {0x005e1a38, "USA UNA TARJETA DE MEMORIA", ""},
    {0x005e1a3c, "(8MB) (p)", "NECESITAS AL MENOS"},
    {0x005e1a40, "CON %iKB DE ESPACIO LIBRE PARA", "%iKB DE ESPACIO LIBRE PARA"},
    {0x005e1a44, "GRABAR TU PROGRESO", nullptr},
    {0x005e1a48, "INTRODUCE UNA TARJETA DE MEMORIA", ""},
    {0x005e1a4c, "(8MB) (p)", "LIBERA AL MENOS"},
    {0x005e1a50, "CON AL MENOS %iKB O", "%iKB DE ESPACIO O"},
    {0x005e1a54, "NO PODR\xa0""S GRABAR", nullptr},
    {0x005e1a58, "ER IS EEN MEMORY CARD", ""},
    {0x005e1a5c, "(8MB) (p)", "JE HEBT MINSTENS"},
    {0x005e1a60, "MET MINSTENS %iKB RUIMTE NODIG OM", "%iKB VRIJE RUIMTE NODIG OM"},
    {0x005e1a64, "JE GAME OP TE SLAAN", nullptr},
    {0x005e1a68, "PLAATS EEN MEMORY CARD", ""},
    {0x005e1a6c, "(8MB) (p)", "MAAK MINSTENS"},
    {0x005e1a70, "MET MINSTENS %iKB OF JE KUNT", "%iKB RUIMTE VRIJ OF JE KUNT"},
    {0x005e1a74, "NIET OPSLAAN", nullptr},
    // The options' ADJUST SCREEN (the TV picture's position) is the PC menus' settings instead (src/menus.cpp)
    {0x005e14a8, "ADJUST SCREEN", "PC SETTINGS"},
    {0x005e14ac, "CENTRER L'ECRAN", "OPTIONS PC"},
    {0x005e14b0, "BILDSCHIRM", "PC-OPTIONEN"},
    {0x005e14b4, "REGOLA SCHERMO", "OPZIONI PC"},
    {0x005e14b8, "AJUSTES DE IMAGEN", "OPCIONES PC"},
    {0x005e14bc, "SCHERM AANPASSEN", "PC-INSTELLINGEN"},
};

u32 Read32(u32 address)
{
    u32 v;
    std::memcpy(&v, Ps2::EeMemory(address), 4);
    return v;
}

std::string ReadString(u32 address) { return reinterpret_cast<const char*>(Ps2::EeMemory(address)); }
}

void WocPcText()
{
    // Every slot as the disc has it, or nothing is done
    for (const Line& line : Lines)
    {
        if (ReadString(Read32(line.slot)) != line.original)
        {
            Ps2::Log("PC text: the line at %08x isn't \"%s\": the console's texts are left as they are", line.slot, line.original);
            return;
        }
    }
    // The space: the replaced lines' own strings (each its length and terminator), less those a kept line still uses
    std::set<u32> kept;
    for (const Line& line : Lines)
    {
        if (line.text == nullptr)
        {
            kept.insert(Read32(line.slot));
        }
    }
    std::map<u32, u32> space; // address -> bytes
    for (const Line& line : Lines)
    {
        u32 at = Read32(line.slot);
        if (line.text != nullptr && !kept.count(at) && line.original[0] != 0)
        {
            space[at] = static_cast<u32>(std::strlen(line.original)) + 1;
        }
    }
    std::vector<std::pair<u32, u32>> free(space.begin(), space.end());
    std::map<std::string, u32> written;
    auto place = [&](const std::string& text) -> u32 {
        auto same = written.find(text);
        if (same != written.end())
        {
            return same->second;
        }
        u32 need = static_cast<u32>(text.size()) + 1;
        // The smallest piece it fits in, the rest of the piece kept for others
        size_t best = free.size();
        for (size_t n = 0; n < free.size(); n++)
        {
            if (free[n].second >= need && (best == free.size() || free[n].second < free[best].second))
            {
                best = n;
            }
        }
        if (best == free.size())
        {
            return 0;
        }
        u32 at = free[best].first;
        std::memcpy(Ps2::EeMemory(at), text.c_str(), need);
        free[best].first += need;
        free[best].second -= need;
        written[text] = at;
        return at;
    };
    // The longest first, so the big pieces go to the lines that need them
    std::vector<const Line*> order;
    for (const Line& line : Lines)
    {
        if (line.text != nullptr)
        {
            order.push_back(&line);
        }
    }
    std::stable_sort(order.begin(), order.end(),
                     [](const Line* a, const Line* b) { return std::strlen(a->text) > std::strlen(b->text); });
    std::map<u32, u32> pointers;
    for (const Line* line : order)
    {
        u32 at = place(line->text);
        if (at == 0)
        {
            Ps2::Log("PC text: no room for \"%s\"", line->text);
            return;
        }
        pointers[line->slot] = at;
    }
    for (const auto& [slot, at] : pointers)
    {
        std::memcpy(Ps2::EeMemory(slot), &at, 4);
    }
}
