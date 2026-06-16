from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import random

class MockAPI(BaseHTTPRequestHandler):
    
    # MockAPI para local, se uso por que se demoraba demasiado al llamar a la api
    def send_json_response(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))

    def do_POST(self):
        # Endpoint de Login: Retorna el JWT que el C++ espera encontrar
        if self.path == '/cpyd/v1/login/authenticate':
            # Formato exacto que tu C++ requiere para el .find("eyJ")
            token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.simulacion.local"
            self.send_json_response({"token": token})
        else:
            self.send_error(404)

    def do_GET(self):
        # Endpoint de Persona: Retorna el género aleatorio
        if self.path.startswith('/cpyd/v1/person/'):
            gender = random.choice(["FEMENINO", "MASCULINO"])
            self.send_json_response({"gender": gender})
        else:
            self.send_error(404)

    # Suprime logs de consola para ver solo el progreso del C++
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    server_address = ('localhost', 8080)
    httpd = HTTPServer(server_address, MockAPI)
    print("Iniciando Mock API local en el puerto 8080...")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido.")
        httpd.server_close()
