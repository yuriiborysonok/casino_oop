
echo "Compiling CasinoPlatform using g++..."

g++ -std=c++20 \
    -I./include \
    -I/opt/homebrew/include \
    -I/opt/homebrew/opt/libpq/include \
    -I./build/_deps/nlohmann_json-src/include \
    -I./build/_deps/httplib-src \
    src/main.cpp src/Wallet.cpp \
    -L/opt/homebrew/lib \
    -L/opt/homebrew/opt/libpq/lib \
    -lpqxx -lpq \
    -o casino_server_gpp

if [ $? -eq 0 ]; then
    echo "✅ Success! Executable created: ./casino_server_gpp"
else
    echo "❌ Compilation failed."
fi
