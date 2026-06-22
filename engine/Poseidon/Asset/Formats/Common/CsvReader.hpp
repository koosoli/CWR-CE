#pragma once

#include <Poseidon/IO/Streams/QStream.hpp>
#include <string>
#include <vector>

namespace Poseidon::Asset::Formats
{

// Read the next cell from stream. After return, the stream is positioned at the
// start of the next cell (past the comma), at end-of-line (CR/LF not consumed),
// or at EOF.
std::string CsvReadCell(QIStream& stream);

// Read all cells on the current line into a row vector.
// Consumes the line terminator (CR, LF, CRLF). Returns false at EOF.
bool CsvReadRow(QIStream& stream, std::vector<std::string>& row);

// Read all rows from stream into a 2D vector.
std::vector<std::vector<std::string>> CsvReadAll(QIStream& stream);

} // namespace Poseidon::Asset::Formats
