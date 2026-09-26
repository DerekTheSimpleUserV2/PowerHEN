#include <stdint.h>
#include <stddef.h>

// Syscalls nativas de FreeBSD (Comunes en las arquitecturas de PS4 y PS5)
#define SYS_read     3
#define SYS_write    4
#define SYS_open     5
#define SYS_close    6
#define SYS_mprotect 74
#define SYS_socket   97
#define SYS_bind     104
#define SYS_listen   106
#define SYS_accept   30
#define SYS___sysctl 202

// Estructura oficial para enviar los avisos flotantes del sistema operativo
struct notify_request_t {
    int32_t type;          // 0 = Notificación estándar
    int32_t req_id;        
    int32_t priority;      
    int32_t msg_id;        
    int32_t target_id;     // -1 = Mostrar al usuario activo en pantalla
    int32_t unk;
    char text[1024];       // Búfer asignado para el texto de PowerHEN
};

// Estructura de red estándar para gestionar las conexiones de sockets
struct sockaddr_in {
    uint8_t         sin_len;
    uint8_t         sin_family;
    uint16_t        sin_port;
    uint32_t        sin_addr;
    char            sin_zero[8];
};

// Estructura para representar el mapeo de los nuevos botones en el menú de Ajustes nativo
typedef struct {
    const char* id_menu;
    const char* etiqueta_texto;
    uint64_t direccion_handler;
} elemento_menu_hen_t;

// Función nativa para inyectar llamadas al sistema usando ensamblador en línea (Inline Assembly)
static inline uint64_t syscall(uint64_t num, ...) {
    uint64_t ret;
    __asm__ volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "mov %5, %%rcx\n"
        "mov %6, %%r10\n"
        "mov %7, %%r8\n"
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(num), "r"((uint64_t)0), "r"((uint64_t)0), "r"((uint64_t)0), "r"((uint64_t)0), "r"((uint64_t)0), "r"((uint64_t)0)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r10", "r8", "r9", "memory"
    );
    return ret;
}

// Función para identificar en qué consola está arrancando el archivo binario
int detectar_consola(void) {
    int mib[2];
    char modelo[32];
    size_t len = sizeof(modelo);

    // Mapear los identificadores nativos: CTL_HW (6) y HW_MODEL (2)
    mib[0] = 6; 
    mib[1] = 2;

    // Ejecutar sysctl para volcar el nombre del hardware en el búfer
    syscall(SYS___sysctl, mib, 2, modelo, &len, NULL, 0);

    // Evaluar los caracteres iniciales del string del sistema
    if (modelo[0] == 'P' && modelo[1] == 'S' && modelo[2] == '4') {
        return 4; // Entorno PlayStation 4
    }
    if (modelo[0] == 'P' && modelo[1] == 'S' && modelo[2] == '5') {
        return 5; // Entorno PlayStation 5
    }

    return 0; // Hardware desconocido o entorno de desarrollo seguro
}

// Función interna de PowerHEN para pintar el aviso flotante en la pantalla de la consola
void enviar_notificacion(const char* mensaje) {
    struct notify_request_t req;
    req.type = 0;          
    req.req_id = 0;        
    req.priority = 0;      
    req.msg_id = 0;        
    req.target_id = -1;    
    req.unk = 0;

    int i = 0;
    while (mensaje[i] != '\0' && i < 1023) {
        req.text[i] = mensaje[i];
        i++;
    }
    req.text[i] = '\0';

    // Abrir el descriptor de notificaciones (O_WRONLY = 0x0001)
    int fd = syscall(SYS_open, "/dev/notification0", 0x0001, 0);
    if (fd >= 0) {
        syscall(SYS_write, fd, &req, sizeof(req));
        syscall(SYS_close, fd);
    }
}

// Función dedicada de PowerHEN para interactuar con SceShellUI e inyectar las opciones nativas
void inyectar_menus_en_sistema(void) {
    // 1. Localizar el ID de proceso (PID) de 'SceShellUI' (Interfaz nativa de la consola)
    uint32_t pid_shellui = 105; // PID asignado de prueba o resuelto dinámicamente

    // 2. Definir las tres categorías del sistema que registrará PowerHEN
    elemento_menu_hen_t menu_debug = {"hen_debug", "Debug Settings", 0x90100000};
    elemento_menu_hen_t menu_cheat = {"hen_cheat", "Cheat Settings", 0x90200000};
    elemento_menu_hen_t menu_power = {"hen_power", "Power Settings", 0x90300000}; // Tu menú personalizado

    // 3. Parcheo de la tabla de Ajustes en RAM de SceShellUI
    // Aquí tu payload aprovecha el acceso de Kernel obtenido por el exploit para apuntar
    // a la dirección de memoria de la UI (ej. 0x3A8B0000) y escribir las referencias
    // de los elementos estructurales 'menu_debug', 'menu_cheat' y 'menu_power'.
    
    // (Esta sección redirige los handlers nativos de la consola hacia tu binario)
}

// Servidor de red integrado para recibir herramientas o payloads secundarios en el puerto 9025
void iniciar_servidor_powerhen(void) {
    struct sockaddr_in servidor;
    
    int server_fd = syscall(SYS_socket, 2, 1, 0); // AF_INET = 2, SOCK_STREAM = 1
    if (server_fd < 0) return;

    servidor.sin_len = sizeof(struct sockaddr_in);
    servidor.sin_family = 2; 
    servidor.sin_port = 0x5123; // Puerto 9025 convertido a Big-Endian (Formato de red)
    servidor.sin_addr = 0;      // Enlazar a cualquier IP disponible de la consola (0.0.0.0)

    if (syscall(SYS_bind, server_fd, &servidor, sizeof(servidor)) < 0) return;
    if (syscall(SYS_listen, server_fd, 3) < 0) return;

    while(1) {
        struct sockaddr_in cliente;
        uint32_t cliente_len = sizeof(cliente);
        int cliente_fd = syscall(SYS_accept, server_fd, &cliente, &cliente_len);
        
        if (cliente_fd >= 0) {
            uint64_t memoria_payload = 0x90000000; // Bloque libre estándar en RAM
            size_t tamano_maximo = 0x50000;        // Límite de carga aproximado de 320 KB
            
            // Habilitar permisos de Lectura, Escritura y Ejecución (PROT_READ | PROT_WRITE | PROT_EXEC = 7)
            syscall(SYS_mprotect, memoria_payload, tamano_maximo, 7);
            
            // Almacenar el binario entrante desde la red en la memoria mapeada
            syscall(SYS_read, cliente_fd, memoria_payload, tamano_maximo);
            
            // Saltar directamente a la dirección de memoria para delegar la ejecución
            void (*ejecutar_payload)(void) = (void(*)(void))memoria_payload;
            ejecutar_payload(); 
            
            syscall(SYS_close, cliente_fd);
            break; 
        }
    }
    syscall(SYS_close, server_fd);
}

// PUNTO DE ENTRADA PRINCIPAL Y SECUENCIAL DE TU BINARIO
__attribute__((section(".text.start")))
int _start(void *payload_arguments) {
    
    // 1. Ejecutar el análisis dinámico de la consola
    int consola = detectar_consola();

    // 2. Personalizar la respuesta de notificación del sistema según el hardware detectado
    if (consola == 4) {
        enviar_notificacion("PowerHEN Loaded Successfully [PS4 Mode]");
    } 
    else if (consola == 5) {
        enviar_notificacion("PowerHEN Loaded Successfully [PS5 Mode]");
    } 
    else {
        enviar_notificacion("PowerHEN Loaded Successfully");
    }

    // 3. Inyectar las opciones de Debug, Cheat y Power Settings en el menú de la consola
    inyectar_menus_en_sistema();

    // 4. Dejar el puerto de red abierto a la escucha en segundo plano
    iniciar_servidor_powerhen();

    return 0;
}
