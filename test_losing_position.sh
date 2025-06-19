#!/bin/bash

# Test script to check if the MCTS engine evaluates losing positions correctly
# Set up a position where white has only a king and black has king + 8 pawns

echo "Testing MCTS engine with clearly losing position..."
echo "Position: White King only vs Black King + 8 pawns"
echo ""

# Create a simple UCI test
echo "uci" | timeout 10s ./montecraplo > /tmp/uci_test.log 2>&1 &
sleep 1

# Test the losing position: fen with white king vs black king + 8 pawns
# This FEN represents: white king on h1, black king on h8, black pawns on a7,b7,c7,d7,e7,f7,g7,h7
FEN="7k/pppppppp/8/8/8/8/8/7K w - - 0 1"

(
echo "uci"
echo "ucinewgame" 
echo "position fen $FEN"
echo "go movetime 3000"
echo "quit"
) | timeout 15s ./montecraplo

echo ""
echo "Test completed. The engine should show a strongly negative evaluation for white."
