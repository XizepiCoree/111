#pragma once

//==============================================================================
// Вопросы секретной викторины. Каждая героиня: два лёгких вопроса по сюжету
// визуальной новеллы, три варианта ответа, correct = номер верного (с нуля).
// Порядок героинь совпадает с кнопками первого вопроса. Правите тексты здесь.
//==============================================================================
struct QuizQuestion
{
    const char* q;
    const char* a[3];
    int correct;
};

struct QuizGirl
{
    const char* name;
    QuizQuestion q[2];
};

inline constexpr const char* quizFirstQuestion = "Who is your favourite female MC?";

inline const QuizGirl quizGirls[6] =
{
    { "Nagisa", {
        { "What club does Nagisa want to revive?",       { "Music club", "Drama club", "Art club" }, 1 },
        { "What does Nagisa's family run?",              { "A bakery", "A flower shop", "A ramen shop" }, 0 } } },

    { "Fuko", {
        { "What does Fuko carve and give to people?",    { "Paper cranes", "Clay cats", "Wooden starfish" }, 2 },
        { "Whose wedding does Fuko invite everyone to?", { "Nagisa's", "Her sister Kouko's", "Tomoyo's" }, 1 } } },

    { "Kyou", {
        { "What does Kyou throw at people when angry?",  { "Dictionaries", "Pillows", "Chalk" }, 0 },
        { "Who is Kyou's twin sister?",                  { "Kotomi", "Fuko", "Ryou" }, 2 } } },

    { "Ryou", {
        { "What is Ryou fond of, even if her results are often wrong?", { "Cooking", "Fortune telling", "Swimming" }, 1 },
        { "What pet does Ryou's twin sister Kyou keep?", { "A baby boar", "A cat", "A parrot" }, 0 } } },

    { "Kotomi", {
        { "Which instrument does Kotomi play?",          { "Piano", "Flute", "Violin" }, 2 },
        { "Where does Kotomi spend most of her time at school?", { "The library", "The gym", "The roof" }, 0 } } },

    { "Tomoyo", {
        { "What does Tomoyo want to become?",            { "Class rep", "Student council president", "Manga artist" }, 1 },
        { "What does Tomoyo want to protect?",           { "The old school building", "The school pond", "The cherry trees on the hill" }, 2 } } },
};
