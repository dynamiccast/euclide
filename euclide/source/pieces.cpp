#include "pieces.h"
#include "problem.h"
#include "tables/tables.h"
#include "cache.h"

namespace Euclide
{

/* -------------------------------------------------------------------------- */

Piece::Piece(const Problem& problem, Square square)
{
	assert(problem.initialPosition(square));

	/* -- Piece characteristics -- */

	m_glyph = problem.initialPosition(square);
	m_color = Euclide::color(m_glyph);
	m_species = problem.piece(m_glyph);

	m_royal = (m_species == King);

	/* -- Initial square is known, final square not -- */

	m_initialSquare = square;
	m_castlingSquare = square;
	m_finalSquare = Nowhere;

	/* -- Has the piece been captured or promoted? -- */

	m_captured = (m_royal || !problem.capturedPieces(m_color)) ? tribool(false) : unknown;
	m_promoted = (m_species == Pawn) ? unknown : tribool(false);

	m_glyphs.set(m_glyph);
	if (maybe(m_captured))
		m_glyphs.set(Empty);
	if (maybe(m_promoted))
		m_glyphs.set([&](Glyph glyph) { return Euclide::color(glyph) == m_color; });

	/* -- Initialize number of available moves and captures -- */

	m_availableMoves = problem.moves(m_color);
	m_availableCaptures = problem.initialPieces(!m_color) - problem.diagramPieces(!m_color);

	m_requiredMoves = 0;
	m_requiredCaptures = 0;

	/* -- Initialize possible final squares and capture squares -- */

	for (Square square : AllSquares())
		m_possibleSquares.set(square, maybe(m_captured) || (problem.diagramPosition(square) == m_glyph) || (maybe(m_promoted) && (Euclide::color(problem.diagramPosition(square)) == m_color)));

	if (m_availableCaptures)
		m_possibleCaptures.set();

	/* -- Initialize legal moves and move tables -- */

	Tables::initializeLegalMoves(&m_moves, m_species, m_color, problem.variant(), m_availableCaptures ? unknown : tribool(false));
	m_xmoves = Tables::getCaptureMoves(m_species, m_color, problem.variant());

	m_constraints = Tables::getMoveConstraints(m_species, problem.variant(), false);
	m_xconstraints = Tables::getMoveConstraints(m_species, problem.variant(), true);

	m_checks = Tables::getUnstoppableChecks(m_species, m_color, problem.variant());

	/* -- Initialize occupied squares -- */

	for (Square square : AllSquares())
		m_occupied[square].pieces.fill(nullptr);

	/* -- Distances will be computed later -- */

	m_distances.fill(0);
	m_captures.fill(0);
	m_rdistances.fill(0);
	m_rcaptures.fill(0);

	/* -- Squares crossed will also be filled later -- */

	m_stops.set();
	m_route.set();

	/* -- Handle castling -- */

	std::fill_n(m_castling, NumCastlingSides, false);

	if ((m_glyph == WhiteKing) || (m_glyph == BlackKing))
		for (CastlingSide side : AllCastlingSides())
			if (m_initialSquare == Castlings[m_color][side].from)
				if (problem.castling(m_color, side))
					m_moves[Castlings[m_color][side].from][Castlings[m_color][side].to] = true, m_castling[side] = unknown;

	if ((m_glyph == WhiteRook) || (m_glyph == BlackRook))
		for (CastlingSide side : AllCastlingSides())
			if (m_initialSquare == Castlings[m_color][side].rook)
				if (problem.castling(m_color, side))
					m_castlingSquare = Castlings[m_color][side].free, m_castling[side] = unknown;

	/* -- Update possible moves -- */

	m_update = true;
	update();
}

/* -------------------------------------------------------------------------- */

Piece::~Piece()
{
}

/* -------------------------------------------------------------------------- */

void Piece::setCastling(CastlingSide side, bool castling)
{
	if (!unknown(m_castling[side]))
		return;

	/* -- King can only castle on one side -- */

	if (castling && m_royal)
		for (CastlingSide other : AllCastlingSides())
			if (side != other)
				setCastling(other, false);

	/* -- Prohibit castling moves -- */

	if (!castling)
	{
		if (m_royal)
			m_moves[Castlings[m_color][side].from][Castlings[m_color][side].to] = false;

		m_castlingSquare = m_initialSquare;
	}

	/* -- Update state -- */

	m_castling[side] = castling;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::setCaptured(bool captured)
{
	if (!unknown(m_captured))
		return;

	m_captured = captured;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::setAvailableMoves(int availableMoves)
{
	if (availableMoves >= m_availableMoves)
		return;

	m_availableMoves = availableMoves;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::setAvailableCaptures(int availableCaptures)
{
	if (availableCaptures >= m_availableCaptures)
		return;

	m_availableCaptures = availableCaptures;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::setPossibleSquares(const Squares& squares)
{
	if ((m_possibleSquares & squares) == m_possibleSquares)
		return;

	m_possibleSquares &= squares;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::setPossibleCaptures(const Squares& squares)
{
	if ((m_possibleCaptures & squares) == m_possibleCaptures)
		return;

	m_possibleCaptures &= squares;
	m_update = true;
}

/* -------------------------------------------------------------------------- */

void Piece::bypassObstacles(const Piece& blocker)
{
	const Squares& obstacles = blocker.stops();

	/* -- Blocked movements -- */

	if ((obstacles & m_route).any())
		for (Square from : ValidSquares(m_stops))
			for (Square to : ValidSquares(m_moves[from]))
				if (obstacles <= ((*m_constraints)[from][to] | from))
					m_moves[from][to] = false, m_update = true;

	/* -- Castling -- */

	if (m_castlingSquare != m_initialSquare)
		for (CastlingSide side : AllCastlingSides())
			if (obstacles <= (*m_constraints)[Castlings[m_color][side].rook][Castlings[m_color][side].free])
				setCastling(side, false);

	/* -- Checks -- */

	if (m_royal && (blocker.m_color != m_color) && (obstacles.count() == 1))
		for (Square check : ValidSquares((*blocker.m_checks)[obstacles.first()]))
			if (m_route[check])
				for (Square from : ValidSquares(m_stops))
					if (m_moves[from][check])
						m_moves[from][check] = false, m_update = true;
}

/* -------------------------------------------------------------------------- */

int Piece::mutualInteractions(Piece& pieceA, Piece& pieceB, const array<int, NumColors>& freeMoves, bool fast)
{
	const int requiredMoves = pieceA.m_requiredMoves + pieceB.m_requiredMoves;
	const bool enemies = pieceA.m_color != pieceB.m_color;

	/* -- Don't bother if these two pieces can not interact with each other -- */

	const Squares routes[2] = {
		pieceA.m_route | ((enemies && pieceB.m_royal) ? pieceA.m_threats : Squares()),
		pieceB.m_route | ((enemies && pieceA.m_royal) ? pieceB.m_threats : Squares())
	};

	if (!(routes[0] & routes[1]))
		return requiredMoves;

	/* -- Use fast method if the search space is too large -- */

	const int threshold = 5000;
	if (pieceA.nmoves() * pieceB.nmoves() > threshold)
		fast = true;

	/* -- Play all possible moves with these two pieces -- */

	array<State, 2> states = {
		State(pieceA, pieceA.m_requiredMoves + freeMoves[pieceA.m_color]),
		State(pieceB, pieceB.m_requiredMoves + freeMoves[pieceB.m_color])
	};

	const int availableMoves = requiredMoves + freeMoves[pieceA.m_color] + (enemies ? freeMoves[pieceB.m_color] : 0);

	TwoPieceCache cache;
	const int newRequiredMoves = fast ? fastplay(states, availableMoves, cache) : fullplay(states, availableMoves, availableMoves, cache);

	if (newRequiredMoves >= Infinity)
		throw NoSolution;

	/* -- Store required moves for each piece, if greater than the previously computed values -- */

	for (const State& state : states)
		if (state.requiredMoves > state.piece.m_requiredMoves)
			state.piece.m_requiredMoves = state.requiredMoves, state.piece.m_update = true;

	/* -- Early exit if we have not performed all computations -- */

	if (fast)
		return newRequiredMoves;

	/* -- Remove never played moves and keep track of occupied squares -- */

	for (const State& state : states)
	{
		for (Square square : AllSquares())
		{
			if (state.moves[square] < state.piece.m_moves[square])
				state.piece.m_moves[square] = state.moves[square], state.piece.m_update = true;

			if (state.squares[square].count() == 1)
			{
				const Square occupied = state.squares[square].first();
				if (!state.piece.m_occupied[square].squares[occupied])
				{
					state.piece.m_occupied[square].squares[occupied] = true;
					state.piece.m_occupied[square].pieces[occupied] = &states[&state == &states[0]].piece;
					state.piece.m_update = true;
				}
			}

			if (state.distances[square] > state.piece.m_distances[square])
				state.piece.m_distances[square] = state.distances[square], state.piece.m_update = true;
		}
	}

	/* -- Done -- */

	return newRequiredMoves;
}

/* -------------------------------------------------------------------------- */

bool Piece::update()
{
	if (!m_update)
		return false;

	updateDeductions();
	m_update = false;

	return true;
}

/* -------------------------------------------------------------------------- */

void Piece::updateDeductions()
{
	/* -- Castling for rooks -- */

	if (m_castlingSquare != m_initialSquare)
		if (!m_moves[m_castlingSquare] && !m_possibleSquares[m_castlingSquare])
			for (CastlingSide side : AllCastlingSides())
				setCastling(side, false);

	if (m_castlingSquare != m_initialSquare)
		if (!m_moves[m_initialSquare] && !m_possibleSquares[m_initialSquare])
			for (CastlingSide side : AllCastlingSides())
				setCastling(side, true);

	if (m_castlingSquare != m_initialSquare)
		if (m_distances[m_castlingSquare])
			for (CastlingSide side : AllCastlingSides())
				setCastling(side, false);

	/* -- Compute distances -- */

	const bool castling = xstd::any_of(AllCastlingSides(), [&](CastlingSide side) { return is(m_castling[side]); });

	updateDistances(castling);
	if (m_xmoves)
		m_captures = computeCaptures(castling ? m_castlingSquare : m_initialSquare, m_castlingSquare);

	for (Square square : ValidSquares(m_possibleSquares))
		if (m_distances[square] > m_availableMoves)
			m_possibleSquares[square] = false;

	for (Square square : ValidSquares(m_possibleCaptures))
		if (m_captures[square] > m_availableCaptures)
			m_possibleCaptures[square] = false;

	m_rdistances = computeDistancesTo(m_possibleSquares);
	if (m_xmoves)
		m_rcaptures = computeCapturesTo(m_possibleSquares);

	/* -- Are there any possible final squares left? -- */

	if (!m_possibleSquares)
		throw NoSolution;

	if (m_possibleSquares.count() == 1)
		m_finalSquare = m_possibleSquares.first();

	/* -- Compute minimum number of moves and captures performed by this piece -- */

	xstd::maximize(m_requiredMoves, xstd::min(ValidSquares(m_possibleSquares), [&](Square square) { return m_distances[square]; }));
	xstd::maximize(m_requiredCaptures, xstd::min(ValidSquares(m_possibleSquares), [&](Square square) { return m_captures[square]; }));

	/* -- Remove moves that will obviously never be played -- */

	for (Square from : AllSquares())
		for (Square to : ValidSquares(m_moves[from]))
			if (m_distances[from] + 1 + m_rdistances[to] > m_availableMoves)
				m_moves[from][to] = false;

	if (m_xmoves)
		for (Square from : AllSquares())
			for (Square to : ValidSquares(m_moves[from]))
				if (m_captures[from] + (*m_xmoves)[from][to] + m_rcaptures[to] > m_availableCaptures)
					m_moves[from][to] = false;

	/* -- Update castling state according to corresponding king moves -- */

	if (m_royal)
	{
		for (CastlingSide side : AllCastlingSides())
		{
			if (maybe(m_castling[side]))
			{
				if (!m_moves[Castlings[m_color][side].from][Castlings[m_color][side].to])
					setCastling(side, false);

				if (m_moves[Castlings[m_color][side].from].count() == 1)
					if (m_moves[Castlings[m_color][side].from][Castlings[m_color][side].to])
						setCastling(side, true);
			}
		}
	}

	/* -- Get all squares the piece may have crossed or stopped on -- */

	m_stops = m_possibleSquares;
	m_stops.set(m_initialSquare);
	m_stops.set(m_castlingSquare);
	for (Square from : AllSquares())
		m_stops |= m_moves[from];

	m_route = m_stops;
	for (Square from : AllSquares())
		for (Square to : ValidSquares(m_moves[from]))
			m_route |= (*m_constraints)[from][to];

	m_threats.reset();
	for (Square square : ValidSquares(m_stops))
		m_threats |= (*m_checks)[square];

	/* -- Update occupied squares -- */

	for (Square square : AllSquares())
	{
		for (bool loop = true; loop; )
		{
			loop = false;
			for (Square occupied : ValidSquares(m_occupied[square].squares))
			{
				for (Square other : ValidSquares(m_occupied[square].pieces[occupied]->m_occupied[occupied].squares))
				{
					if (!m_occupied[square].squares[other])
					{
						m_occupied[square].pieces[other] = m_occupied[square].pieces[occupied]->m_occupied[occupied].pieces[other];
						m_occupied[square].squares[other] = true;
						loop = true;
					}
				}
			}
		}
	}

}

/* -------------------------------------------------------------------------- */

void Piece::updateDistances(bool castling)
{
	const array<int, NumSquares> distances = computeDistances(castling ? m_castlingSquare : m_initialSquare, m_castlingSquare);

	for (Square square : AllSquares())
		xstd::maximize(m_distances[square], distances[square]);
}

/* -------------------------------------------------------------------------- */

array<int, NumSquares> Piece::computeDistances(Square square, Square castling) const
{
	/* -- Initialize distances -- */

	array<int, NumSquares> distances;
	distances.fill(Infinity);
	distances[square] = 0;
	distances[castling] = 0;

	/* -- Initialize square queue -- */

	Queue<Square, NumSquares> squares;
	squares.push(square);
	if (castling != square)
		squares.push(castling);

	/* -- Loop until every reachable square has been handled -- */

	while (!squares.empty())
	{
		const Square from = squares.front(); squares.pop();

		/* -- Handle every possible immediate destination -- */

		for (Square to : ValidSquares(m_moves[from]))
		{
			/* -- This square may have been attained by a quicker path -- */

			if (distances[to] < Infinity)
				continue;

			/* -- Set square distance -- */

			distances[to] = distances[from] + 1;

			/* -- Add it to queue of reachable squares -- */

			squares.push(to);
		}
	}

	/* -- Done -- */

	return distances;
}

/* -------------------------------------------------------------------------- */

array<int, NumSquares> Piece::computeDistancesTo(Squares destinations) const
{
	/* -- Initialize distances and square queue -- */

	array<int, NumSquares> distances;
	Queue<Square, NumSquares> squares;

	distances.fill(Infinity);

	for (Square square : AllSquares())
		if ((distances[square] = destinations[square] ? 0 : Infinity) == 0)
			squares.push(square);

	/* -- Loop until every reachable square has been handled -- */

	while (!squares.empty())
	{
		const Square to = squares.front(); squares.pop();

		/* -- Handle every possible immediate destination -- */

		for (Square from : AllSquares())
		{
			/* -- Skip illegal moves -- */

			if (!m_moves[from][to])
				continue;

			/* -- This square may have been attained by a quicker path -- */

			if (distances[from] < Infinity)
				continue;

			/* -- Set square distance -- */

			distances[from] = distances[to] + 1;

			/* -- Add it to queue of reachable squares -- */

			squares.push(from);
		}
	}

	/* -- Done -- */

	return distances;
}

/* -------------------------------------------------------------------------- */

array<int, NumSquares> Piece::computeCaptures(Square square, Square castling) const
{
	assert(m_xmoves);

	/* -- Initialize captures -- */

	array<int, NumSquares> captures;
	captures.fill(Infinity);
	captures[square] = 0;
	captures[castling] = 0;

	/* -- Initialize square queue -- */

	Queue<Square, NumSquares> squares;
	squares.push(square);
	if (castling != square)
		squares.push(castling);

	/* -- Loop until every reachable square has been handled -- */

	while (!squares.empty())
	{
		const Square from = squares.front(); squares.pop();

		/* -- Handle every possible immediate destination -- */

		for (Square to : ValidSquares(m_moves[from]))
		{
			/* -- This square may have been attained using less captures -- */

			const int required = captures[from] + (*m_xmoves)[from][to];
			if (required >= captures[to])
				continue;

			/* -- Set required captures -- */

			captures[to] = required;

			/* -- Add it to queue of squares -- */

			squares.push(to);
		}
	}

	/* -- Done -- */

	return captures;
}

/* -------------------------------------------------------------------------- */

array<int, NumSquares> Piece::computeCapturesTo(Squares destinations) const
{
	/* -- Initialize captures and square queue -- */

	array<int, NumSquares> captures;
	Queue<Square, NumSquares> squares;

	captures.fill(Infinity);

	for (Square square : AllSquares())
		if ((captures[square] = destinations[square] ? 0 : Infinity) == 0)
			squares.push(square);

	/* -- Loop until every reachable square has been handled -- */

	while (!squares.empty())
	{
		const Square to = squares.front(); squares.pop();

		/* -- Handle every possible immediate destination -- */

		for (Square from : AllSquares())
		{
			/* -- Skip illegal moves -- */

			if (!m_moves[from][to])
				continue;

			/* -- This square may have been attained using less captures -- */

			const int required = captures[to] + (*m_xmoves)[from][to];
			if (required >= captures[from])
				continue;

			/* -- Set square required number of captures -- */

			captures[from] = required;

			/* -- Add it to queue of reachable squares -- */

			squares.push(from);
		}
	}

	/* -- Done -- */

	return captures;
}

/* -------------------------------------------------------------------------- */

int Piece::fastplay(array<State, 2>& states, int availableMoves, TwoPieceCache& cache)
{
	typedef TwoPieceCache::Position Position;
	Queue<Position, 8 * NumSquares * NumSquares> queue;

	int requiredMoves = Infinity;

	const bool friends = (states[0].piece.m_color == states[1].piece.m_color);
	const bool partners = friends && (states[0].piece.m_royal || states[1].piece.m_royal) && (states[0].teleportation || states[1].teleportation);

	/* -- Initial position -- */

	Position initial(states[0].piece.m_initialSquare, 0, states[1].piece.m_initialSquare, 0);
	queue.push(initial);
	cache.add(initial);

	/* -- Loop -- */

	for ( ; !queue.empty(); queue.pop())
	{
		const Position& position = queue.front();

		/* -- Check if we have reached our goal -- */

		if (states[0].piece.m_possibleSquares[position.squares[0]] && states[1].piece.m_possibleSquares[position.squares[1]])
		{
			/* -- Get required moves -- */

			xstd::minimize(states[0].requiredMoves, position.moves[0]);
			xstd::minimize(states[1].requiredMoves, position.moves[1]);
			xstd::minimize(requiredMoves, position.moves[0] + position.moves[1]);
		}

		/* -- Play all moves -- */

		for (int s = 0; s < 2; s++)
		{
			const int k = s ^ (position.moves[0] > position.moves[1]);

			State& state = states[k];
			State& xstate = states[k ^ 1];
			Piece& piece = state.piece;
			Piece& xpiece = xstate.piece;
			const Square from = position.squares[k];
			const Square other = position.squares[k ^ 1];

			/* -- Handle teleportation for rooks -- */

			if (state.teleportation && !position.moves[k] && (position.squares[k] == piece.m_initialSquare))
			{
				Square to = piece.m_castlingSquare;

				/* -- Teleportation could be blocked by other piece -- */

				const bool blocked = (*piece.m_constraints)[from][to][other];
				if (!blocked)
				{
					Position next(k ? other : to, position.moves[0], k ? to : other, position.moves[1]);
					if (!cache.hit(next))
					{
						/* -- Insert resulting position in front of queue -- */

						queue.pass(next, 1);
						cache.add(next);
					}
				}
			}

			/* -- Check if there are any moves left for this piece -- */

			if (state.availableMoves <= position.moves[k])
				continue;

			if ((state.requiredMoves <= position.moves[k]) && (xstate.requiredMoves <= position.moves[k ^ 1]))
				continue;

			/* -- Check that the enemy is not in check -- */

			if (xpiece.m_royal && !friends && (*piece.m_checks)[from][other])
				continue;

			/* -- Loop over all moves -- */

			for (Square to : ValidSquares(piece.m_moves[from]))
			{
				Position next(k ? other : to, position.moves[0] + (k ^ 1), k ? to : other, position.moves[1] + (k ^ 0));

				/* -- Take castling into account -- */

				if (piece.m_royal && !position.moves[k] && partners)
					for (CastlingSide side : AllCastlingSides())
						if ((to == Castlings[piece.m_color][side].to) && (other == Castlings[piece.m_color][side].rook) && !position.moves[k ^ 1])
							next.squares[k ^ 1] = Castlings[piece.m_color][side].free;

				/* -- Continue if position was already reached before -- */

				if (cache.hit(next))
					continue;

				/* -- Move could be blocked by other pieces -- */

				bool blocked = (*piece.m_constraints)[from][to][other] || xpiece.m_occupied[other].squares[from];
				for (Square square : ValidSquares(xpiece.m_occupied[other].squares))
					if ((*piece.m_constraints)[from][to][square])
						blocked = true;

				if (blocked)
					continue;

				/* -- Reject move if it brings us to far away -- */

				if (piece.m_rdistances[to] > std::min(availableMoves, state.availableMoves - next.moves[k]))
					continue;

				/* -- Reject move if we move into check -- */

				if (piece.m_royal && !friends && (*xpiece.m_checks)[other][to])
					continue;

				/* -- Castling constraints -- */

				if (piece.m_royal && !friends && (from == piece.m_initialSquare))
					for (CastlingSide side : AllCastlingSides())
						if ((Castlings[piece.m_color][side].from == from) && (Castlings[piece.m_color][side].to == to))
							if (position.moves[k] || (*xpiece.m_checks)[other][from] || (*xpiece.m_checks)[other][Castlings[piece.m_color][side].free])
								continue;

				/* -- Safeguard if maximum queue size is insufficient -- */

				assert(!queue.full());
				if (queue.full())
				{
					states[0].requiredMoves = states[0].piece.m_requiredMoves;
					states[1].requiredMoves = states[1].piece.m_requiredMoves;
					return states[0].requiredMoves + states[1].requiredMoves;
				}

				/* -- Play move and add it to cache -- */

				queue.push(next);
				cache.add(next);
			}
		}
	}

	/* -- Done -- */

	return requiredMoves;
}

/* -------------------------------------------------------------------------- */

int Piece::fullplay(array<State, 2>& states, int availableMoves, int maximumMoves, TwoPieceCache& cache, bool *invalidate)
{
	int requiredMoves = Infinity;

	/* -- Check if we have achieved our goal -- */

	if (states[0].piece.m_possibleSquares[states[0].square] && states[1].piece.m_possibleSquares[states[1].square])
	{
		/* -- Get required moves -- */

		xstd::minimize(states[0].requiredMoves, states[0].playedMoves);
		xstd::minimize(states[1].requiredMoves, states[1].playedMoves);
		requiredMoves = states[0].playedMoves + states[1].playedMoves;

		/* -- Label occupied squares -- */

		states[0].squares[states[0].square][states[1].square] = true;
		states[1].squares[states[1].square][states[0].square] = true;
	}

	/* -- Break recursion if there are no more moves available -- */

	if (availableMoves < 0)
		return requiredMoves;

	/* -- Check for cache hit -- */

	if (cache.hit(states[0].square, states[0].playedMoves, states[1].square, states[1].playedMoves, &requiredMoves))
		return requiredMoves;

	/* -- Loop over both pieces -- */

	for (int k = 0; k < 2; k++)
	{
		const int s = k ^ (states[0].playedMoves > states[1].playedMoves);

		State& state = states[s];
		State& xstate = states[s ^ 1];
		Piece& piece = state.piece;
		Piece& xpiece = xstate.piece;
		const Square from = state.square;
		const Square other = xstate.square;
		const bool friends = (piece.m_color == xpiece.m_color);

		/* -- Teleportation when castling -- */

		if (state.teleportation && !state.playedMoves)
		{
			const Square king = (xpiece.m_royal && friends) ? other : Nowhere;
			const Square pivot = std::find_if(Castlings[piece.m_color], Castlings[piece.m_color] + NumCastlingSides, [=](const Castling& castling) { return castling.rook == from; })->to;

			if ((king != Nowhere) ? (king == pivot) && (xstate.playedMoves == 1) : !(*piece.m_constraints)[piece.m_initialSquare][piece.m_castlingSquare][other])
			{
				assert(!piece.m_distances[piece.m_castlingSquare]);
				if (!piece.m_distances[piece.m_castlingSquare])
				{
					state.square = piece.m_castlingSquare;
					state.teleportation = false;

					const int myRequiredMoves = fullplay(states, availableMoves, maximumMoves, cache);
					if (myRequiredMoves <= maximumMoves)
					{
						state.squares[from][other] = true;
						xstate.squares[other][from] = true;

						state.distances[piece.m_castlingSquare] = 0;
					}

					xstd::minimize(requiredMoves, myRequiredMoves);

					state.teleportation = true;
					state.square = piece.m_initialSquare;
				}
			}
		}

		/* -- Check if there are any moves left for this piece -- */

		if (state.availableMoves <= 0)
			continue;

		/* -- Check that the enemy is not in check -- */

		if (xpiece.m_royal && !friends && (*piece.m_checks)[from][other])
			continue;

		/* -- Loop over all moves -- */

		for (Square to : ValidSquares(piece.m_moves[from]))
		{
			/* -- Move could be blocked by other pieces -- */

			bool blocked = (*piece.m_constraints)[from][to][other] || xpiece.m_occupied[other].squares[from];
			for (Square square : ValidSquares(xpiece.m_occupied[other].squares))
				if ((*piece.m_constraints)[from][to][square])
					blocked = true;

			if (blocked)
				continue;

			/* -- Reject move if it brings us to far away -- */

			if (1 + piece.m_rdistances[to] > std::min(availableMoves, state.availableMoves))
				continue;

			/* -- Reject move if we have taken a shortcut -- */

			if (state.playedMoves + 1 < piece.m_distances[to])
				if (!invalidate || ((*invalidate = true), true))
					continue;

			/* -- Reject move if we move into check -- */

			if (piece.m_royal && !friends && (*xpiece.m_checks)[other][to])
				continue;

			/* -- Castling constraints -- */

			if (piece.m_royal && !friends && (from == piece.m_initialSquare))
				for (CastlingSide side : AllCastlingSides())
					if ((Castlings[piece.m_color][side].from == from) && (Castlings[piece.m_color][side].to == to))
						if (state.playedMoves || (*xpiece.m_checks)[other][from] || (*xpiece.m_checks)[other][Castlings[piece.m_color][side].free])
							continue;

			/* -- Play move -- */

			state.availableMoves -= 1;
			state.playedMoves += 1;
			state.square = to;

			/* -- Recursion -- */

			bool shortcuts = false;
			const int myRequiredMoves = fullplay(states, availableMoves - 1, maximumMoves, cache, &shortcuts);

			/* -- Cache this position, for tremendous speedups -- */

			cache.add(states[0].square, states[0].playedMoves, states[1].square, states[1].playedMoves, myRequiredMoves, shortcuts);

			/* -- Label all valid moves that can be used to reach our goals and squares that were occupied -- */

			if (myRequiredMoves <= maximumMoves)
			{
				state.moves[from][to] = true;
				state.squares[from][other] = true;
				xstate.squares[other][from] = true;

				xstd::minimize(state.distances[to], state.playedMoves);
			}

			/* -- Undo move -- */

			state.availableMoves += 1;
			state.playedMoves -= 1;
			state.square = from;

			/* -- Update required moves -- */

			xstd::minimize(requiredMoves, myRequiredMoves);
		}
	}

	/* -- Done -- */

	return requiredMoves;
}

/* -------------------------------------------------------------------------- */

}
