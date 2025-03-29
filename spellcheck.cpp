#include "spellcheck.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <set>
#include <vector>
#include <numeric>
#include <cctype>
#include <iterator>
#include <optional>

namespace rv = std::ranges::views;
template <typename Iterator, typename UnaryPred>
std::vector<Iterator> find_all(Iterator begin, Iterator end, UnaryPred pred);

Corpus tokenize(const std::string& source) {
    Corpus tokens;
    auto whitespace_iters = find_all(source.begin(), source.end(), ::isspace);
    
    std::transform(
        whitespace_iters.begin(), whitespace_iters.end() - 1,  
        whitespace_iters.begin() + 1,                          
        std::inserter(tokens, tokens.begin()),                 
        [&source](auto begin_it, auto end_it) {                
            return Token(source, begin_it, end_it);
        }
    );

    std::erase_if(tokens, [](const Token& t) { 
        return t.content.empty(); 
    });
    
    return tokens;
}

std::set<Mispelling> spellcheck(const Corpus& source, const Dictionary& dictionary) {
    auto is_misspelled = [&](const Token& token) {
        if (token.content.empty()) {
            return false;
        }
        if (dictionary.count(token.content)) {
            return false;
        }
        if (isupper(token.content[0]) && 
            std::all_of(token.content.begin()+1, token.content.end(), islower)) {
            return false;
        }
        return true;
    };

    auto generate_suggestions = [&](const Token& token) {
        std::set<std::string> suggestions;
        std::ranges::for_each(dictionary, [&](const auto& word) {
            if (levenshtein(token.content, word) <= 1) {
                suggestions.insert(word);
            }
        });
        return suggestions;
    };

    auto process_token = [&](const Token& token) {
        auto suggestions = generate_suggestions(token);
        return Mispelling{token, suggestions};
    };

    auto misspelled = source | rv::filter(is_misspelled);
    auto processed = misspelled | rv::transform(process_token);
    auto with_suggestions = processed | rv::filter([](const Mispelling& m) { 
        return !m.suggestions.empty(); 
    });
    
    return {with_suggestions.begin(), with_suggestions.end()};
}

#include "utils.cpp"
