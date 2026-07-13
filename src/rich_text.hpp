#pragma once

#include "raylib.h"
#include <string>
#include <vector>

enum class BlockType
{
    Paragraph,
    Heading1,
    Heading2,
    Heading3,
    BulletList,
    NumberedList
};

struct RichTextRun
{
    std::string text;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    Color textColor = WHITE;
    Color highlightColor = {0, 0, 0, 0};

    bool hasFormatting() const
    {
        return bold || italic || underline || strikethrough ||
               textColor.r != 255 || textColor.g != 255 ||
               textColor.b != 255 || textColor.a != 255 ||
               highlightColor.a != 0;
    }
};

struct RichTextBlock
{
    BlockType type = BlockType::Paragraph;
    std::vector<RichTextRun> runs;

    std::string getText() const;
    void setText(const std::string& t);

    bool empty() const { return runs.empty() || (runs.size() == 1 && runs[0].text.empty()); }
};

std::string blockTypeToMarkdownPrefix(BlockType type);
BlockType markdownPrefixToBlockType(const std::string& prefix);

std::string richTextToMarkdown(const std::vector<RichTextBlock>& blocks);
std::vector<RichTextBlock> markdownToRichText(const std::string& md);

std::string colorToHex(Color c);
Color hexToColor(const std::string& hex);
