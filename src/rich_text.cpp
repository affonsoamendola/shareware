#include "rich_text.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

// RichTextBlock helpers

std::string RichTextBlock::getText() const
{
    std::string result;
    for (auto& run : runs)
    {
        result += run.text;
    }
    return result;
}

void RichTextBlock::setText(const std::string& t)
{
    runs.clear();
    RichTextRun run;
    run.text = t;
    runs.push_back(run);
}

// Block type to/from markdown prefix

std::string blockTypeToMarkdownPrefix(BlockType type)
{
    switch (type)
    {
        case BlockType::Heading1:     return "# ";
        case BlockType::Heading2:     return "## ";
        case BlockType::Heading3:     return "### ";
        case BlockType::BulletList:   return "- ";
        case BlockType::NumberedList: return "1. ";
        default:                      return "";
    }
}

BlockType markdownPrefixToBlockType(const std::string& prefix)
{
    if (prefix == "# ")      return BlockType::Heading1;
    if (prefix == "## ")     return BlockType::Heading2;
    if (prefix == "### ")    return BlockType::Heading3;
    if (prefix == "- ")      return BlockType::BulletList;
    return BlockType::Paragraph;
}

// Color hex conversion

std::string colorToHex(Color c)
{
    char buf[10];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    return std::string(buf);
}

Color hexToColor(const std::string& hex)
{
    if (hex.size() < 7) return WHITE;
    unsigned int r = 0, g = 0, b = 0;
    sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
    return {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
}

// Encode a single run to markdown

static std::string runToMarkdown(const RichTextRun& run)
{
    std::string text = run.text;
    if (text.empty()) return "";

    bool hasColor = (run.textColor.r != 255 || run.textColor.g != 255 ||
                     run.textColor.b != 255 || run.textColor.a != 255);
    bool hasHighlight = (run.highlightColor.a != 0);

    std::string prefix, suffix;

    if (run.bold)           { prefix += "**"; suffix = "**" + suffix; }
    if (run.italic)         { prefix += "*";  suffix = "*"  + suffix; }
    if (run.underline)      { prefix += "<u>"; suffix = "</u>" + suffix; }
    if (run.strikethrough)  { prefix += "~~";  suffix = "~~"  + suffix; }
    if (hasHighlight)
    {
        std::string hex = colorToHex(run.highlightColor);
        prefix += "<mark color=\"" + hex + "\">";
        suffix = "</mark>" + suffix;
    }
    if (hasColor)
    {
        std::string hex = colorToHex(run.textColor);
        prefix += "<font color=\"" + hex + "\">";
        suffix = "</font>" + suffix;
    }

    return prefix + text + suffix;
}

// Rich text to markdown

std::string richTextToMarkdown(const std::vector<RichTextBlock>& blocks)
{
    std::string result;
    int numberedIndex = 1;

    for (size_t i = 0; i < blocks.size(); i++)
    {
        auto& block = blocks[i];

        if (block.empty())
        {
            if (i + 1 < blocks.size())
            {
                result += "\n";
            }
            numberedIndex = 1;
            continue;
        }

        std::string prefix = blockTypeToMarkdownPrefix(block.type);

        if (block.type == BlockType::NumberedList)
        {
            char numBuf[8];
            snprintf(numBuf, sizeof(numBuf), "%d. ", numberedIndex);
            prefix = numBuf;
            numberedIndex++;
        }
        else
        {
            numberedIndex = 1;
        }

        std::string lineText;
        for (auto& run : block.runs)
        {
            lineText += runToMarkdown(run);
        }

        result += prefix + lineText;
        if (i + 1 < blocks.size())
        {
            result += "\n";
        }
    }

    return result;
}

// Parse inline formatting from a string into runs

static std::vector<RichTextRun> parseInlineRuns(const std::string& text)
{
    std::vector<RichTextRun> runs;
    size_t pos = 0;

    while (pos < text.size())
    {
        RichTextRun run;

        // Check for bold+italic ***text***
        if (pos + 2 < text.size() && text[pos] == '*' && text[pos+1] == '*' && text[pos+2] == '*')
        {
            size_t end = text.find("***", pos + 3);
            if (end != std::string::npos)
            {
                run.bold = true;
                run.italic = true;
                run.text = text.substr(pos + 3, end - pos - 3);
                runs.push_back(run);
                pos = end + 3;
                continue;
            }
        }

        // Check for bold **text**
        if (pos + 1 < text.size() && text[pos] == '*' && text[pos+1] == '*')
        {
            size_t end = text.find("**", pos + 2);
            if (end != std::string::npos)
            {
                run.bold = true;
                run.text = text.substr(pos + 2, end - pos - 2);
                runs.push_back(run);
                pos = end + 2;
                continue;
            }
        }

        // Check for italic *text*
        if (text[pos] == '*')
        {
            size_t end = text.find('*', pos + 1);
            if (end != std::string::npos)
            {
                run.italic = true;
                run.text = text.substr(pos + 1, end - pos - 1);
                runs.push_back(run);
                pos = end + 1;
                continue;
            }
        }

        // Check for strikethrough ~~text~~
        if (pos + 1 < text.size() && text[pos] == '~' && text[pos+1] == '~')
        {
            size_t end = text.find("~~", pos + 2);
            if (end != std::string::npos)
            {
                run.strikethrough = true;
                run.text = text.substr(pos + 2, end - pos - 2);
                runs.push_back(run);
                pos = end + 2;
                continue;
            }
        }

        // Check for <u>text</u>
        if (pos + 2 < text.size() && text.substr(pos, 3) == "<u>")
        {
            size_t end = text.find("</u>", pos + 3);
            if (end != std::string::npos)
            {
                run.underline = true;
                run.text = text.substr(pos + 3, end - pos - 3);
                runs.push_back(run);
                pos = end + 4;
                continue;
            }
        }

        // Check for <font color="hex">text</font>
        if (pos + 13 < text.size() && text.substr(pos, 13) == "<font color=\"")
        {
            size_t closeQuote = text.find("\"", pos + 13);
            size_t closeTag = text.find(">", closeQuote);
            if (closeQuote != std::string::npos && closeTag != std::string::npos)
            {
                std::string hex = text.substr(pos + 13, closeQuote - pos - 13);
                size_t endFont = text.find("</font>", closeTag + 1);
                if (endFont != std::string::npos)
                {
                    run.textColor = hexToColor(hex);
                    run.text = text.substr(closeTag + 1, endFont - closeTag - 1);
                    runs.push_back(run);
                    pos = endFont + 7;
                    continue;
                }
            }
        }

        // Check for <mark color="hex">text</mark>
        if (pos + 14 < text.size() && text.substr(pos, 14) == "<mark color=\"")
        {
            size_t closeQuote = text.find("\"", pos + 14);
            size_t closeTag = text.find(">", closeQuote);
            if (closeQuote != std::string::npos && closeTag != std::string::npos)
            {
                std::string hex = text.substr(pos + 14, closeQuote - pos - 14);
                size_t endMark = text.find("</mark>", closeTag + 1);
                if (endMark != std::string::npos)
                {
                    run.highlightColor = hexToColor(hex);
                    run.highlightColor.a = 128;
                    run.text = text.substr(closeTag + 1, endMark - closeTag - 1);
                    runs.push_back(run);
                    pos = endMark + 7;
                    continue;
                }
            }
        }

        // Plain character - accumulate until next special char
        size_t nextSpecial = text.size();
        for (size_t i = pos + 1; i < text.size(); i++)
        {
            if (text[i] == '*' || text[i] == '~' ||
                (i + 2 < text.size() && text.substr(i, 3) == "<u>") ||
                (i + 12 < text.size() && text.substr(i, 13) == "<font color=\"") ||
                (i + 13 < text.size() && text.substr(i, 14) == "<mark color=\""))
            {
                nextSpecial = i;
                break;
            }
        }

        run.text = text.substr(pos, nextSpecial - pos);
        runs.push_back(run);
        pos = nextSpecial;
    }

    if (runs.empty())
    {
        RichTextRun run;
        run.text = "";
        runs.push_back(run);
    }

    return runs;
}

// Markdown to rich text blocks

std::vector<RichTextBlock> markdownToRichText(const std::string& md)
{
    std::vector<RichTextBlock> blocks;
    std::istringstream stream(md);
    std::string line;
    int numIndex = 1;

    while (std::getline(stream, line))
    {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        RichTextBlock block;

        // Detect numbered list
        bool isNumbered = false;
        if (line.size() > 2 && std::isdigit(line[0]) && line[1] == '.' && line[2] == ' ')
        {
            block.type = BlockType::NumberedList;
            line = line.substr(3);
            isNumbered = true;
        }
        // Detect other prefixes
        else if (line.size() >= 3 && line.substr(0, 3) == "### ")
        {
            block.type = BlockType::Heading3;
            line = line.substr(4);
        }
        else if (line.size() >= 3 && line.substr(0, 3) == "## ")
        {
            block.type = BlockType::Heading2;
            line = line.substr(3);
        }
        else if (line.size() >= 2 && line.substr(0, 2) == "# ")
        {
            block.type = BlockType::Heading1;
            line = line.substr(2);
        }
        else if (line.size() >= 2 && line.substr(0, 2) == "- ")
        {
            block.type = BlockType::BulletList;
            line = line.substr(2);
        }
        else
        {
            block.type = BlockType::Paragraph;
        }

        block.runs = parseInlineRuns(line);
        blocks.push_back(block);
    }

    if (blocks.empty())
    {
        RichTextBlock block;
        block.type = BlockType::Paragraph;
        RichTextRun run;
        run.text = "";
        block.runs.push_back(run);
        blocks.push_back(block);
    }

    return blocks;
}
