cd "$(dirname "$0")"

./stop.sh 2>/dev/null || true
sleep 1

mkdir -p logs

cd service
node server.js > ../logs/service.log 2>&1 &

cd ../frontend
npm run start > ../logs/frontend.log 2>&1 &

cd ../gateway
node gateway.js > ../logs/gateway.log 2>&1 &
cd ..

