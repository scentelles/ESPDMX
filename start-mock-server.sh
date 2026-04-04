#!/usr/bin/env bash
# Start mock server for local development

echo "Building frontend..."
cd frontend
npm run build
cd ..

echo ""
echo "Starting mock server..."
node mock-server.js
