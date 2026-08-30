set -e
cd "$(dirname "$0")"

cd engine
make
cd ..

cd gateway
npm install --silent
cd ..

cd service
npm install --silent
cd ..

cd frontend
npm install --silent
npm run build
cd ..