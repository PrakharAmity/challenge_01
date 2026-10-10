#include "match_data.hpp"

namespace cricpulse {

MatchFixture getSampleMatchFixture() {
    MatchFixture fixture;
    fixture.matchId = "IND-AUS-2026-FINAL";
    fixture.matchTitle = "India vs Australia • World Championship Final";
    fixture.matchStatus = "LIVE - 1st Innings";
    fixture.battingTeam = "India";
    fixture.bowlingTeam = "Australia";
    fixture.totalRuns = 155;
    fixture.wickets = 3;
    fixture.legalBalls = 75; // 12.3 overs

    // 12 completed overs: evaluates sliding windows for best stretch
    // Windows:
    // 1-6: 86
    // 2-7: 88
    // 3-8: 95
    // 4-9: 98
    // 5-10: 110
    // 6-11: 112 (selected if final window is skipped)
    // 7-12: 117 (actual highest 6-over stretch)
    fixture.overRuns = {11, 14, 16, 12, 15, 18, 13, 21, 19, 24, 17, 23};

    // Recent ball-by-ball commentary feed
    fixture.ballFeed = {
        {12, 3, "Ravindra Jadeja", "Mitchell Starc", 2, false, "Driven firmly into the gap at deep cover for a brisk brace."},
        {12, 2, "Ravindra Jadeja", "Mitchell Starc", 4, false, "FOUR! Flashed hard through backward point, beats third man to the rope."},
        {12, 1, "KL Rahul", "Mitchell Starc", 0, true, "WICKET! Edged and taken behind! Splendid outswinger from Starc."},
        {11, 6, "Virat Kohli", "Pat Cummins", 1, false, "Tucked off the hips towards backward square leg to retain strike."},
        {11, 5, "Virat Kohli", "Pat Cummins", 6, false, "SIX! Signature whip off the pads over deep midwicket, sailed into the stands!"},
        {11, 4, "Virat Kohli", "Pat Cummins", 2, false, "Punched down the ground past the non-striker for two comfortable runs."},
        {11, 3, "KL Rahul", "Pat Cummins", 1, false, "Pushed towards cover-point with soft hands for a quick single."},
        {11, 2, "KL Rahul", "Pat Cummins", 4, false, "FOUR! Short of length, pulled emphatically through midwicket."},
        {11, 1, "Virat Kohli", "Pat Cummins", 1, false, "Back of a length angling in, deflected to third man for one."},
        {10, 6, "Virat Kohli", "Adam Zampa", 1, false, "Milked to long-on to complete another productive over."},
        {10, 5, "KL Rahul", "Adam Zampa", 1, false, "Driven down to long-off, rotating the strike cleanly."},
        {10, 4, "KL Rahul", "Adam Zampa", 4, false, "FOUR! Uses the feet and lofts cleanly inside-out over extra cover."},
        {10, 3, "KL Rahul", "Adam Zampa", 0, false, "Beaten on the drive outside off, good variation from Zampa."},
        {10, 2, "Virat Kohli", "Adam Zampa", 1, false, "Worked away past square leg for a single."},
        {10, 1, "Virat Kohli", "Adam Zampa", 2, false, "Full on middle, clipped through midwicket for a couple."}
    };

    // Team players
    fixture.players = {
        {0, "Rohit Sharma", "Opening Batter", "India"},
        {1, "Virat Kohli", "Top-order Batter", "India"},
        {2, "KL Rahul", "Wicketkeeper Batter", "India"},
        {3, "Shubman Gill", "Opening Batter", "India"},
        {4, "Ravindra Jadeja", "All-Rounder", "India"},
        {5, "Hardik Pandya", "All-Rounder", "India"},
        {6, "Jasprit Bumrah", "Fast Bowler", "India"}
    };

    // Partnership graph (7 players: 0 to 6)
    // Directed progression graph reflecting batting partnership sequence:
    // (0, 1) wt 15
    // (1, 4) wt 7
    // (0, 2) wt 4
    // (2, 3) wt 4
    // (3, 4) wt 8
    // (4, 5) wt 6
    // (5, 6) wt 5
    fixture.partnershipGraph.resize(7);

    auto addProgressionEdge = [&](int u, int v, int w) {
        fixture.partnershipGraph[u].push_back({v, w});
    };

    addProgressionEdge(0, 1, 15);
    addProgressionEdge(1, 4, 7);
    addProgressionEdge(0, 2, 4);
    addProgressionEdge(2, 3, 4);
    addProgressionEdge(3, 4, 8);
    addProgressionEdge(4, 5, 6);
    addProgressionEdge(5, 6, 5);

    // Fan poll candidates
    fixture.pollOptions = {
        {"opt-1", "Virat Kohli (58* off 37)", 1420},
        {"opt-2", "Rohit Sharma (47 off 24)", 980},
        {"opt-3", "Mitchell Starc (2/28 in 3.3 ov)", 640},
        {"opt-4", "Ravindra Jadeja (18* off 8)", 310}
    };

    return fixture;
}

} // namespace cricpulse
