#pragma once

//==============================================================================
// Реплики, которые появляются по клику на фон.
//
// Это НЕ дословные цитаты из игры: тексты написаны заново в характере героинь
// (оригинальный текст новеллы защищён авторским правом). Чтобы добавить свои
// реплики, допишите строки в конец списка в формате { "Имя", "Текст" }.
// Текст помещается примерно в две строки диалогового окна.
//==============================================================================
struct Phrase
{
    const char* speaker;
    const char* text;
};

inline const Phrase phrases[] =
{
    // Нагиса: робкая, упрямая, любит хлеб и данго
    { "Nagisa", "Um... I'll do my best. Really! Even if my voice shakes a little." },
    { "Nagisa", "If this kick had a heartbeat, it would be a very nervous one." },
    { "Nagisa", "Dango, dango... ah! Was I humming out loud again?" },
    { "Nagisa", "Even if it's hard, I want to try it one more time." },
    { "Nagisa", "Dad's bread would taste wonderful with this beat. Probably. Maybe." },

    // Фуко: мечтательная, вырезает звёзды из дерева
    { "Fuko", "I carved a little starfish for this song. Please take good care of it." },
    { "Fuko", "That sound is like a wooden starfish: small, but made with lots of love." },
    { "Fuko", "Hm? Do you hear that? Someone far away is cheering for your kick." },
    { "Fuko", "I was thinking about the school festival. Mostly. A little." },

    // Кё: прямолинейная старшая сестра со словарём
    { "Kyou", "Don't just stand there. Turn that knob already!" },
    { "Kyou", "If it sounds weird, don't blame me. I'm not the one holding the mouse." },
    { "Kyou", "Hmph. It's not bad. Not that I'm praising you or anything." },
    { "Kyou", "Keep this up and I'll have to bring out the dictionary." },
    { "Kyou", "Come on, put some guts into it. A kick should have guts!" },

    // Рё: мягкая, гадает на картах, легко смущается
    { "Ryou", "Um, my cards say this kick is going to hit very, very hard today." },
    { "Ryou", "I... I think it's wonderful. Honestly." },
    { "Ryou", "Maybe a little less drive? Ah, sorry, it's your music!" },
    { "Ryou", "The cards were drawn twice, and both times they said: more bass." },

    // Котоми: застенчивый гений из библиотеки
    { "Kotomi", "Mathematically, that low end is very pleasing." },
    { "Kotomi", "In a library, silence is a sound too. Please be gentle with the volume." },
    { "Kotomi", "I'd like to hear it once more, if that's alright." },
    { "Kotomi", "I counted the beats. There were exactly as many as I hoped for." },

    // Томоё: сильная, уверенная, старшая в совете
    { "Tomoyo", "A kick that can't knock down a door isn't a kick. Turn it up." },
    { "Tomoyo", "I'll deal with the noisy ones. You just focus on the sound." },
    { "Tomoyo", "No sweat. Rhythm is just a fight you win with timing." },
    { "Tomoyo", "Not bad at all. Do it again, but with more spirit." },
};

inline constexpr int numPhrases = (int) (sizeof (phrases) / sizeof (phrases[0]));
