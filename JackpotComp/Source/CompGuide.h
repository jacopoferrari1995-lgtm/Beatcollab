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
          "COMPRESS. Alza PEAK RED. finche vedi 3-6 dB di riduzione sui passaggi forti, poi recupera col GAIN. Per la voce rap 4-6 dB." },
        { "Ottico piu rapido (1.5 ms) e piu presente nelle medie, rilascio piu corto. Tiene sotto controllo senza ammorbidire troppo.",
          "Chitarre, voce davanti al mix, rullante, synth lead.",
          "COMPRESS, 3-5 dB di riduzione. LIMIT quando devi bloccare picchi ballerini." },
        { "FET velocissimo e aggressivo: attacco da 20 us, rilascio da 50 ms. Aggiunge grinta e distorsione dispari quando lavora tanto. ALL = tutti i pulsanti premuti: schiaccia e distorce.",
          "Voce rap (prima del 2A), rullante, room di batteria, basso, 808 per tenerlo davanti.",
          "Ratio 4, attacco medio (300 us), rilascio veloce (100-200 ms). Alza INPUT fino a 5-8 dB di GR. Per la room batteria: ALL + mix al 30-40%." },
        { "VCA da bus: preciso, pulito, 'colla'. Attacco e rilascio a scatti come l'originale, AUTO segue il materiale.",
          "Drum bus, bus strumentale, master (con poca riduzione).",
          "Ratio 2 o 4, attacco 10 o 30 ms per lasciar passare il transiente, rilascio AUTO o 0.1 s a tempo. Threshold per 2-4 dB di GR." },
        { "Valvola a pendenza variabile: piu il segnale e forte piu comprime, ginocchio larghissimo. Tempi a costanti fisse (TC1-TC6), TC5 e TC6 automatici. Spessa e morbida.",
          "Master, mix bus, bus voci, drum bus lo-fi, basso.",
          "TC2 o TC3 per la batteria, TC5 per il master. INPUT per 1-3 dB sul master, 4-6 sui bus. COLOR 0.3-0.5 per il calore." },
        { "RMS: reagisce all'energia e non ai picchi, ginocchio duro, attacco che si adatta al materiale, rilascio a 120 dB/s. Diretto e 'punchy'.",
          "Cassa, basso, 808, rullante in serie dopo un FET.",
          "Ratio 4, threshold per 4-8 dB. Sul basso/808 tiene il livello costante senza mangiare l'attacco." },
        { "Moderno e versatile: va dal quasi trasparente al distrutto. 1 = solo colore, NUKE = limitatore brutale. COLOR sotto 0.5 = distorsione di 2a armonica, sopra = 3a.",
          "Batteria in parallelo, room, voce aggressiva, basso distorto, parallel smash.",
          "Ratio 4, attacco 5-10 ms, rilascio 200-400 ms. Per lo smash parallelo: ratio 20 o NUKE, COLOR 0.6, MIX 25-35%." },
        { "Multibanda su tre bande: schiaccia le parti forti e alza quelle deboli (compressione verso l'alto). Tutto diventa denso, brillante e in faccia. DEPTH dosa l'effetto, ATTACK e RELEASE scalano i tempi delle tre bande.",
          "Hat, synth, pad, voci di sfondo, 808 che deve uscire sulle casse piccole, bus FX. Sul master solo pochissimo.",
          "UP + DOWN, DEPTH 20-40% (100% e l'effetto famoso, molto estremo). Usa MIX 30-50% per tenere la dinamica. Attento al rumore di fondo: l'UP lo alza." },
        { "Il compressore a valvole per eccellenza: 6386 a pendenza variabile, trasformatori grossi. Piu spingi l'INPUT piu il rapporto cresce (da circa 1.3:1 a 20:1), ginocchio larghissimo, attacco rapido ma rotondo. Il trasformatore d'ingresso arrotonda i bassi, la valvola aggiunge 2a armonica che cresce con la riduzione, l'uscita ha un filo di brillantezza: caldo e lucido insieme. STEREO: L/R collegati oppure LAT/VERT (comprime separatamente centro e lati).",
          "Master e mix bus, bus voci, voce solista, batteria intera, basso, pianoforte. Anche solo per il colore con 0-1 dB di riduzione.",
          "TC2 per batteria e voce, TC5 o TC6 (automatici) per master e bus. INPUT per il colore, THRESHOLD per quanto comprime: sul master 1-2 dB, sui bus 2-4. LAT/VERT sul master per tenere fermo il centro e lasciare respirare i lati." },
        { "Multibanda a 2, 3 o 4 bande con crossover Linkwitz-Riley a fase compensata (da spento a 1:1 la somma e piatta). Ogni banda ha soglia, ratio, attacco, rilascio e guadagno suoi, solo e bypass. SOGLIE +/- sposta tutte le soglie insieme, ATTACK e RELEASE dello stadio scalano i tempi di tutte le bande, WARM ammorbidisce l'attacco e scalda come il modo Warm dell'alpha.",
          "Master, mix bus, voce (de-essing largo e controllo del fango), 808 con i bassi che ballano, bus di batteria, chitarre acustiche.",
          "Parti da 3 bande. Bassa: soglia per 2-3 dB, ratio 3, attacco 30 ms, rilascio 200 ms. Media: piu gentile (1-2 dB). Alta: attacco 5 ms per domare le S. Ascolta in SOLO ogni banda mentre regoli." } };
    return G[juce::jlimit (0, 9, m)];
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
