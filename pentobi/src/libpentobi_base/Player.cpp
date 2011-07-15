//-----------------------------------------------------------------------------
/** @file libpentobi_base/Player.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Player.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

Player::Player(const Board& bd)
    : m_bd(bd)
{
}

Player::~Player() throw()
{
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base
