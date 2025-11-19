# Academia Portal

A multi-threaded client-server application for course registration management system developed as a mini-project for CS-513 System Software at IIIT Bangalore.

## Overview

Academia Portal is a Linux-based course registration system that allows students to enroll in courses, faculty to manage their course offerings, and administrators to manage users and oversee the system. The application uses socket programming for client-server communication and implements file locking for concurrent access control.

## Features

### Administrator
- Add new students to the system
- Add new faculty members
- Activate/Deactivate student accounts
- Update student and faculty details
- Manage system-wide operations

### Student
- Enroll in available courses
- Unenroll from registered courses
- View currently enrolled courses
- Change account password

### Faculty
- Add new courses with seat limits
- Remove offered courses
- View student enrollments in courses
- Change account password

## Architecture

The project follows a client-server architecture:
- **Server**: Handles multiple concurrent client connections using multi-threading
- **Client**: Provides command-line interface for user interactions
- **Data Storage**: Uses binary files with file locking for data persistence
- **Communication**: TCP/IP socket programming for network communication

## Technical Highlights

- Uses **system calls** instead of library functions for:
  - Process Management
  - File Management
  - File Locking
  - Multithreading
  - Inter-Process Communication
- Implements **file locking** (using `flock`) for concurrent access control
- **Multi-threaded server** to handle multiple clients simultaneously
- **Password protection** for all user accounts
- **Role-based access control** (Admin, Student, Faculty)

## Requirements

- Linux/Unix operating system
- GCC compiler
- POSIX threads library (pthread)
- Standard C libraries
- Network connectivity for client-server communication

## Project Structure

```
academia-portal/
├── src/
│   ├── server/
│   │   ├── server.c              # Main server implementation
│   │   ├── admin_handler.c       # Admin operations handler
│   │   ├── student_handler.c     # Student operations handler
│   │   ├── faculty_handler.c     # Faculty operations handler
│   │   ├── auth.c               # Authentication module
│   │   └── file_ops.c           # File operations utilities
│   ├── client/
│   │   ├── client.c             # Client implementation
│   │   └── ui.c                 # User interface functions
│   └── common/
│       ├── structures.h         # Data structures definitions
│       ├── constants.h          # Constants and macros
│       └── utils.c              # Common utility functions
├── data/                        # Data files directory
├── Makefile                     # Build configuration
└── README.md                    # This file
```

## Installation & Setup

1. **Compile the project**
   ```bash
   make
   ```
   Or compile manually:
   ```bash
   # Compile server in server directory
   gcc server.c auth.c admin_handler.c student_handler.c faculty_handler.c file_ops.c -lpthread -o server

   # Compile client in academia-portal directory
   gcc -o client src/client/client.c src/client/ui.c -I common 
   ```

2. **Start the server**
   ```bash
   ./server 
   ```
   Default port is typically defined in constants.h if not specified.

3. **Start the client**
   ```bash
   ./client 
   ```
   Default server IP is localhost (127.0.0.1).

## Usage

### First Time Setup
On first run, the server creates a default admin account:
- Username: `admin`
- Password: `admin123`

**Important**: Change the default admin password after first login.

### Client Operations

1. **Login**: Enter username and password when prompted
2. **Select operation**: Choose from the menu based on your role
3. **Follow prompts**: Complete the requested information
4. **Logout**: Select Exit from the menu

### Server Management
- Start server: `./server [port]`
- Stop server: Press `Ctrl+C` (graceful shutdown)
- Monitor logs: Check console output for connection logs

## Data Files

The system stores data in binary format in the `data/` directory:
- `students.dat` - Student records
- `faculty.dat` - Faculty records
- `courses.dat` - Course information
- `enrollments.dat` - Student-course enrollments
- `credentials.dat` - User authentication data

## Implementation Details

### File Locking
- Read operations use shared locks (`LOCK_SH`)
- Write operations use exclusive locks (`LOCK_EX`)
- Ensures data consistency during concurrent access

### Session Management
- Each client connection maintains a session with authentication state
- Sessions are thread-isolated for security

### Error Handling
- Comprehensive error checking for system calls
- Graceful error messages to clients
- Server continues operation even if individual client requests fail

## Security Features

- Password-protected accounts for all users
- Role-based access control
- Session-based authentication
- File permissions for data protection
- Input validation and sanitization

## Limitations & Future Enhancements

### Current Limitations
- Plain text password storage (should use hashing)
- No encryption for network communication
- Limited to local network communication
- Basic user interface (CLI only)

### Potential Enhancements
- Implement password hashing (SHA256 or bcrypt)
- Add SSL/TLS for secure communication
- Create a web-based interface
- Add course prerequisites and scheduling
- Implement grade management
- Add email notifications
- Database integration for better scalability

## Troubleshooting

### Common Issues

1. **Connection refused error**
   - Ensure server is running
   - Check if port is not already in use
   - Verify firewall settings

2. **Permission denied errors**
   - Ensure data directory has write permissions
   - Run with appropriate user privileges

3. **Compilation errors**
   - Verify all required headers are present
   - Check for pthread library linking

## Contributing

This is an academic project. For any issues or improvements:
1. Report bugs through the issue tracker
2. Submit pull requests with clear descriptions
3. Follow the existing code style
4. Add appropriate comments and documentation

## License

This project is developed for educational purposes as part of the CS-513 System Software course at IIIT Bangalore.

## Acknowledgments

- IIIT Bangalore for the project guidelines
- Course instructors for technical guidance
- Unix/Linux system programming resources

---

For more information, refer to the project documentation or contact the development team.