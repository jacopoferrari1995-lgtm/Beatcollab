#pragma once
// Testi della guida: cosa fa ogni modello e come partire
namespace jc
{
struct Guide { const char* carattere; const char* usi; const char* partenza; };

inline const Guide& guide (int m)
{
    static const Guide G[] = {
        { "Ottico a valvole: attacco morbido (10 ms), rilascio in due tempi, prima veloce poi lungo e dipendente da quanto ha lavorato. Non si sente, livella. Colore valvolare caldo.",
          "Voce, basso elettrico, synth pad, bus degli strumenti. Ottimo DOPO un FET 76 in serie.",
          "COMPRESS. Alza PEAK REDUCTION finche vedi 3-6 dB di riduzione sui passaggi forti, poi recupera col GAIN. Per la voce rap 4-6 dB." },
        { "Ottico piu rapido (1.5 ms) e piu presente nelle medie, rilascio piu corto. Tiene sotto controllo senza ammorbidire troppo.",
          "Chitarre, voce davanti al mix, rullante, synth lead.",
          "COMPRESS, 3-5 dB di riduzione. LIMIT quando devi bloccare picchi ballerini." },
        { "FET velocissimo e aggressivo, come l'1176: non ha threshold. INPUT spinge il segnale (e alza il volume) dentro una soglia fissa: piu' INPUT = piu' compressione e piu' grinta; il volume lo riporti con OUTPUT. ATTACK e RELEASE da 1 a 7, come gli altri modelli verso destra piu' lenti (attacco da 20 us a 800 us, rilascio da 50 ms a 1.1 s): stessi tempi dell'1176, solo il verso e' quello degli altri. ALL = tutti i pulsanti premuti: schiaccia e distorce.",
          "Voce rap (prima del 2A), rullante, room di batteria, basso, 808 per tenerlo davanti.",
          "Ratio 4, ATTACK 5, RELEASE 1-2. Alza INPUT finche' il VU segna 3-6 dB sui picchi, poi abbassa OUTPUT di quasi quanto hai alzato. Room batteria: ALL, ATTACK 4, RELEASE 1, MIX 30-40%." },
        { "VCA da bus: preciso, pulito, 'colla'. Attacco e rilascio a scatti come l'originale, AUTO segue il materiale.",
          "Drum bus, bus strumentale, master (con poca riduzione).",
          "Ratio 2 o 4, attacco 10 o 30 ms per lasciar passare il transiente, rilascio AUTO o 0.1 s a tempo. Threshold per 2-4 dB di GR." },
        { "Valvole a pendenza variabile come il Manley: piu' il segnale va oltre la soglia piu' il rapporto cresce (COMPRESS da circa 1.5:1 a 4:1, LIMIT fino a 20:1), ginocchio larghissimo. INPUT spinge le valvole e alza il volume, THRESHOLD decide dove comincia, ATTACK continuo 25-70 ms, RECOVERY a scatti 0.2 / 0.4 / 0.6 / 4 / 8 s. Spessa e morbida.",
          "Master, mix bus, bus voci, drum bus lo-fi, basso.",
          "COMPRESS. Master: RECOVERY 4 s, ATTACK a meta', THRESHOLD per 1-2 dB. Bus: RECOVERY 0.6 s, 3-4 dB. LIMIT per batterie spesse. VALVOLA 30-50% per il calore." },
        { "RMS come il dbx 160: reagisce all'energia e non ai picchi, ginocchio duro, attacco e rilascio decisi dal circuito (attacco che si adatta, rilascio a 120 dB/s). COMPRESSION continua da 1:1 a infinito, come la manopola dell'originale.",
          "Cassa, basso, 808, rullante in serie dopo un FET.",
          "COMPRESSION 4:1, THRESHOLD per 4-8 dB. Sul basso/808 tiene il livello costante senza mangiare l'attacco." },
        { "Come il Distressor: INPUT e OUTPUT (l'INPUT alza il volume e spinge nella soglia fissa), ATTACK e RELEASE da 0 a 10 (verso destra piu' lenti), ratio da 1:1 (solo colore) a NUKE. I pulsanti DIST 2 (2a armonica, calda) e DIST 3 (3a, aggressiva) aggiungono distorsione che cresce col livello; HP DETECTOR fa ignorare i bassi al rilevatore.",
          "Batteria in parallelo, room, voce aggressiva, basso distorto, parallel smash.",
          "Ratio 4, ATTACK 3-5, RELEASE 3-4. Voce: HP acceso. Smash parallelo: ratio 20 o NUKE, DIST 3, MIX 25-35%." },
        { "Multibanda su tre bande: schiaccia le parti forti e alza quelle deboli (compressione verso l'alto). Tutto diventa denso, brillante e in faccia. DEPTH dosa l'effetto, ATTACK e RELEASE scalano i tempi delle tre bande.",
          "Hat, synth, pad, voci di sfondo, 808 che deve uscire sulle casse piccole, bus FX. Sul master solo pochissimo.",
          "UP + DOWN, DEPTH 20-40% (100% e l'effetto famoso, molto estremo). Usa MIX 30-50% per tenere la dinamica. Attento al rumore di fondo: l'UP lo alza." },
        { "Il compressore a valvole per eccellenza: 6386 a pendenza variabile, trasformatori grossi. Piu spingi l'INPUT piu il rapporto cresce (da circa 1.3:1 a 20:1), ginocchio larghissimo, attacco rapido ma rotondo. Il trasformatore d'ingresso arrotonda i bassi, la valvola aggiunge 2a armonica che cresce con la riduzione, l'uscita ha un filo di brillantezza: caldo e lucido insieme. STEREO: L/R collegati oppure LAT/VERT (comprime separatamente centro e lati).",
          "Master e mix bus, bus voci, voce solista, batteria intera, basso, pianoforte. Anche solo per il colore con 0-1 dB di riduzione.",
          "TC2 per batteria e voce, TC5 o TC6 (automatici) per master e bus. INPUT per il colore (alza anche il volume: riportalo con OUTPUT), THRESHOLD per quanto comprime: sul master 1-2 dB, sui bus 2-4. LAT/VERT sul master per tenere fermo il centro e lasciare respirare i lati." },
        { "Multibanda a 2, 3 o 4 bande con crossover Linkwitz-Riley a fase compensata (da spento a 1:1 la somma e piatta). Ogni banda ha soglia, ratio, attacco, rilascio e guadagno suoi, solo e bypass. SOGLIE +/- sposta tutte le soglie insieme, ATTACK e RELEASE dello stadio scalano i tempi di tutte le bande, WARM ammorbidisce l'attacco e scalda come il modo Warm dell'alpha.",
          "Master, mix bus, voce (de-essing largo e controllo del fango), 808 con i bassi che ballano, bus di batteria, chitarre acustiche.",
          "Parti da 3 bande. Bassa: soglia per 2-3 dB, ratio 3, attacco 30 ms, rilascio 200 ms. Media: piu gentile (1-2 dB). Alta: attacco 5 ms per domare le S. Ascolta in SOLO ogni banda mentre regoli." },
        { "VCA del canale SSL 4000 E: preciso e deciso, RATIO continuo da 1:1 a infinito, soglia, rilascio 0.1-4 s; attacco AUTO (si adatta al segnale, da 30 a 3 ms per 20 dB) o FAST ATT (3 ms per 20 dB). Suono moderno, punch con un filo di grana.",
          "Voce pop e rap, rullante, cassa, basso, chitarre: il compressore di canale di mille dischi.",
          "Voce: ratio 4, attacco AUTO, rilascio 0.2-0.4 s, soglia per 4-6 dB. Batteria: FAST spento per lasciare il colpo, ratio 4-6, rilascio corto a tempo." } };
    return G[juce::jlimit (0, 10, m)];
}

// consigli generali sulle tecniche
inline const char* routeTip (int route)
{
    switch (route)
    {
        case 2: return "PARALLELO: tutti gli stadi accesi lavorano sullo stesso segnale e vengono sommati. Usa uno stadio spinto (ALL/NUKE/OTT) e uno delicato, poi dosa con i MIX. SOLO su uno stadio fa sentire cosa sta togliendo.";
        default: return "SERIE: gli stadi accesi lavorano uno dopo l'altro (1, 2, 3...). Classico: FET 76 veloce che prende i picchi (2-4 dB) e OPTO 2A che livella (2-4 dB). Piu stadi leggeri suonano piu naturali di uno spinto. MIX sotto il 100% su uno stadio = compressione parallela (New York).";
    }
}
}
