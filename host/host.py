#!/usr/bin/python3 
from textwrap import fill
import threading
import threading
from tkinter import *
from tkinter import messagebox
import struct
import crcmod
import socket
from tkinter import filedialog
import time


# username and password for login
correct_username = "ibraheem"
correct_password = "qwASZX2004"

# flag for login status
flag=False



def crc32_stm32_style(data):
    CRC = 0xFFFFFFFF
    POLY = 0x04C11DB7

    # mimic HAL_CRC_Accumulate(&input, 1)
    # each byte is treated as a full 32-bit word
    for b in data:
        word = b & 0xFF  # becomes 0x000000b

        CRC ^= word

        for _ in range(32):
            if CRC & 0x80000000:
                CRC = ((CRC << 1) ^ POLY)
            else:
                CRC = (CRC << 1)

            CRC &= 0xFFFFFFFF

    return CRC

#-------------create socket as server-----------------------# 
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(("192.168.1.3", 5000))
server_socket.listen(1)
print("Server is listening on port 5000...")
client_socket, client_address = server_socket.accept()
print(f"Connection from {client_address} has been established!")



#---------------------------------commands------------------------------------------#
bootloader_version_command_length = 0x01 
bootloader_version_command =        0x10

bootloader_chip_id_command_length = 0x01
bootloader_chip_id_command =        0x12


bootloader_get_read_write_protection_status_command_length = 0x01
bootloader_get_read_write_protection_status_command =       0x18

bootloader_read_option_bytes_command_length = 0x01
bootloader_read_option_bytes_command =        0x19

bootloader_erase_command_length = 0x03
bootloader_erase_command =        0x13

bootloader_erase_all_command_length = 0x01
bootloader_erase_all_command =        0x14

bootloader_upload_command_length = 0x06
bootloader_upload_command =        0x17

bootloader_upload_command=0x17
bootloader_upload_command_length_base = 0x06

bootloader_jump_command=0x16
bootloader_jump_command_length=0x05


bootloader_reset_button =0xff


max_size_for_upload =224*1024 # 224 KB in bytes
flash_app_start_address=0x08008000

min_sector_number=2
max_sector_number=5

#---------------------------Button handlers----------------------------#

#----------login button handler----------#
def login():
    global flag
    username = username_entry.get()
    password = password_entry.get()
    if username == correct_username and password == correct_password:
        print("Login successful!")
        flag=True
        login_window.destroy()
        
    else:
        messagebox.showerror("Error", "Invalid username or password.")
        # print("Invalid username or password.")


#----------bootloader version button handler----------#

def bootloader_version_handler():
    def task():
        bootloader_version_lst=[bootloader_version_command_length, bootloader_version_command]
        
        client_socket.send(bytes([bootloader_version_lst[0]]))
        client_socket.send(bytes([bootloader_version_lst[1]]))

        crc = crc32_stm32_style(bytes(bootloader_version_lst))
        client_socket.send(crc.to_bytes(4, byteorder='little'))
        # client_socket.send(bytes([0xFF]))  # End of message
        received_list = list (client_socket.recv(1024))
        print(f"Received response for bootloader version: {received_list}")
        if received_list[0] == 1:
            # print(f"bootloader ID: {received_list[1]}")
            print(f"bootloader version: {received_list[2]}.{received_list[3]}.{received_list[4]}.")
            #clean the log section in the fota system window
            log_text.delete(1.0, END)
            # append to the log section in the fota system window
            log_text.insert(END, f"bootloader ID: {received_list[1]}\n")
            log_text.insert(END, f"bootloader version: {received_list[2]}.{received_list[3]}.{received_list[4]}\n")
        else:
            log_text.delete(1.0, END)
            print("Error in bootloader version response.")
            log_text.insert(END, "Invalid response received.\n")


    threading.Thread(target=task, daemon=True).start()

#----------chip id button handler----------#

def get_chip_id_handler():
    def task():
        chip_id_commands=[bootloader_chip_id_command_length, bootloader_chip_id_command]
        # send the command for get chip id
        client_socket.send(bytes([chip_id_commands[0]]))
        client_socket.send(bytes([chip_id_commands[1]]))

        crc = crc32_stm32_style(bytes(chip_id_commands))
        client_socket.send(crc.to_bytes(4, byteorder='little'))


        received_list= list (client_socket.recv(1024))
        if received_list[0] == 1:
            print(f"Chip ID: {received_list[1]|received_list[2]<<8}")
            log_text.delete(1.0, END)
            log_text.insert(END, f"Chip ID: {received_list[1]|received_list[2]<<8}\n")
        else:
            log_text.delete(1.0, END)
            print("Error in chip ID response.")
            log_text.insert(END, "Error in chip ID response.\n")

    threading.Thread(target=task, daemon=True).start()



#----------read the read write protection status button handler----------#

def get_read_write_protection_status_handler():
    def task():
        read_write_protection_status_commands=[bootloader_get_read_write_protection_status_command_length,
                                                bootloader_get_read_write_protection_status_command]
        # send the command for get read write protection status
        client_socket.send(bytes([read_write_protection_status_commands[0]]))
        client_socket.send(bytes([read_write_protection_status_commands[1]]))

        crc = crc32_stm32_style(bytes(read_write_protection_status_commands))
        client_socket.send(crc.to_bytes(4, byteorder='little'))
        # client_socket.send(bytes([0xFF]))  # End of message
        received_list = list (client_socket.recv(1024))
        if received_list[0] == 1:
            print(f"Read Write Protection Status: {received_list[1]}")
            log_text.delete(1.0, END)
            if (received_list[1] == 0):
                log_text.insert(END, "read write protection: level 0\n")
            elif (received_list[1] == 1):
                log_text.insert(END, "read write protection: level 1\n")
        else:
            log_text.delete(1.0, END)
            print("Error in read write protection status response.")
            log_text.insert(END, "Error in read write protection status response.\n")

    threading.Thread(target=task, daemon=True).start()



#----------button handler for reading option bytes----------#

def read_option_bytes_handler():
    def task():
        read_option_bytes_commands=[bootloader_read_option_bytes_command_length, bootloader_read_option_bytes_command]
        # send the command for reading option bytes
        client_socket.send(bytes([read_option_bytes_commands[0]]))
        client_socket.send(bytes([read_option_bytes_commands[1]]))

        crc = crc32_stm32_style(bytes(read_option_bytes_commands))
        client_socket.send(crc.to_bytes(4, byteorder='little'))
        # client_socket.send(bytes([0xFF]))  # End of message
        received_list = list (client_socket.recv(1024))
        if received_list[0] == 1:
            print(f"Option Bytes: {received_list[1]|received_list[2]<<8|received_list[3]<<16|received_list[4]<<24}")
            log_text.delete(1.0, END)
            log_text.insert(END, f"Option Bytes: {received_list[1]|received_list[2]<<8|received_list[3]<<16|received_list[4]<<24}\n")
        else:
            log_text.delete(1.0, END)
            print("Error in reading option bytes response.")
            log_text.insert(END, "Error in reading option bytes response.\n")

    threading.Thread(target=task, daemon=True).start()


#-----------button handler for erase button-------# 

def erase_button_handler():
    def task():
        sector_number = int(sector_number_spinbox.get())
        number_of_sectors = int(number_of_sectors_spinbox.get())
        if sector_number < min_sector_number or sector_number > max_sector_number or number_of_sectors > (max_sector_number - sector_number + 1):
            log_text.delete(1.0, END)
            print("Error: in selecting the number of sectors.")
            log_text.insert(END, "Error: in selecting the number of sectors.\n")
            return
        erase_command = [bootloader_erase_command_length, bootloader_erase_command, sector_number, number_of_sectors]
        client_socket.send(bytes(erase_command))

        crc = crc32_stm32_style(bytes(erase_command))
        client_socket.send(crc.to_bytes(4, byteorder='little'))

        received_list = list (client_socket.recv(10))
        if received_list[0] == 1:
            print(f"Erase successful for sectors {sector_number} to {sector_number + number_of_sectors - 1}")
            log_text.delete(1.0, END)
            log_text.insert(END, f"Erase successful for sectors {sector_number} to {sector_number + number_of_sectors - 1}\n")
        else:
            log_text.delete(1.0, END)
            print("Error in erase response.")
            log_text.insert(END, "Error in erase response.\n")

    threading.Thread(target=task, daemon=True).start()


#----------button handler for erase all button----------#
def erase_all_button_handler():
    def task():
        erase_all_command = [bootloader_erase_all_command_length, bootloader_erase_all_command]
        client_socket.send(bytes(erase_all_command))

        crc = crc32_stm32_style(bytes(erase_all_command))
        client_socket.send(crc.to_bytes(4, byteorder='little'))

        received_list = list (client_socket.recv(1024))
        if received_list[0] == 1:
            print("Erase all successful.")
            log_text.delete(1.0, END)
            log_text.insert(END, "Erase all successful.\n")
        else:
            log_text.delete(1.0, END)
            print("Error in erase all response.")
            log_text.insert(END, "Error in erase all response.\n")

    threading.Thread(target=task, daemon=True).start()


#------------browse button handler for upload section--------------#
def browse_button_handler():
    def task():
        file_path = filedialog.askopenfilename(
        initialdir="~/Desktop",  
        title="Select a file",
        filetypes=(("All files", "*.*"),)
    )
    
        if file_path:
            file_path_entry.delete(0, END)
            file_path_entry.insert(0, file_path)
    threading.Thread(target=task, daemon=True).start()


#--------upload button handler for upload section--------------#

def upload_flash_button_handler():
    def task():
        file_path = file_path_entry.get()
        if not file_path:
            log_text.delete(1.0, END)
            log_text.insert(END, "Error: No file selected.\n")
            return
        
        try:
            with open(file_path, 'rb') as f:
                file_data = f.read()
            
            max_app_size =max_size_for_upload  
            
            if len(file_data) > max_app_size:
                log_text.delete(1.0, END)
                log_text.insert(END, f"Error: File size ({len(file_data)} bytes) exceeds maximum ({max_app_size} bytes).\n")
                return
            
            log_text.delete(1.0, END)
            log_text.insert(END, f"Starting flash upload. File size: {len(file_data)} bytes.\n")
            
            start_address = flash_app_start_address
            max_packet_size = 248
            offset = 0
            
            while offset < len(file_data):
                chunk_size = min(max_packet_size, len(file_data) - offset)
                chunk = file_data[offset:offset + chunk_size]
                
                # Build write flash packet: length + command + address + chunk_size + data
                address_bytes = start_address.to_bytes(4, byteorder='little')
                write_command = [len(chunk) + bootloader_upload_command_length_base, bootloader_upload_command] + list(address_bytes) + [chunk_size] + list(chunk)
                
                client_socket.send(bytes(write_command))
                crc = crc32_stm32_style(bytes(write_command))
                client_socket.send(crc.to_bytes(4, byteorder='little'))
                
                time.sleep(.1)
                
                received_list = list(client_socket.recv(10))
                if received_list[0] != 1:
                    log_text.delete(1.0, END)
                    log_text.insert(END, f"bl_not_ok: Error writing at address 0x{start_address:08X}.\n")
                    return
                else:
                    log_text.insert(END, f"bl_ok: Written {offset + chunk_size}/{len(file_data)} bytes.\n")
                
                offset += chunk_size
                start_address += chunk_size
                log_text.see(END)
                fota_system_window.update()
            
            # Jump to app
            app_start_address =flash_app_start_address
            jump_command = [bootloader_jump_command_length, bootloader_jump_command] + list(app_start_address.to_bytes(4, byteorder='little'))
            client_socket.send(bytes(jump_command))
            crc = crc32_stm32_style(bytes(jump_command))
            client_socket.send(crc.to_bytes(4, byteorder='little'))

            
            time.sleep(0.5)
            
            received_list = list(client_socket.recv(1024))
            if received_list[0] == 1:
                log_text.insert(END, "bl_ok: Flash upload completed successfully!\n")
            else:
                log_text.delete(1.0, END)
                log_text.insert(END, "bl_not_ok: Error jumping to application.\n")
        
        except Exception as e:
            log_text.delete(1.0, END)
            log_text.insert(END, f"Error: {str(e)}\n")
    threading.Thread(target=task, daemon=True).start()


#--------reset bootloader button handler for upload section--------------#
def reset_bootloader_button_handler():
    def task():
        try:
            client_socket.send(bytes([0x01]))
            client_socket.send(bytes([0xFF]))
            log_text.delete("1.0", END)
            log_text.insert(END, "Reset command sent (len=0x01, cmd=0xFF).\n")
        except Exception as err:
            log_text.delete("1.0", END)
            log_text.insert(END, f"Reset command failed: {err}\n")

    threading.Thread(target=task, daemon=True).start()

#-----------------------login GUI window -----------------------#
login_window = Tk()
login_window.title("Fota system login")
login_window.configure(bg="#2D3B40")
login_window.geometry("400x300+300+300")
login_window.resizable()

# set the main label
Label(login_window, text="Welcome to FOTA system", font=("Arial", 16), bg="#2D3B40", fg="white").pack(pady=20)
#  sest the name place and password place#2D3B40
Label(login_window, text="Username:",font=("Arial", 12), bg="#2D3B40", fg="white").place(x=50, y=70)
Label(login_window, text="Password:",font=("Arial", 12), bg="#2D3B40", fg="white").place(x=50, y=130)

#set the entry for name and password
username_entry = Entry(login_window, font=("Arial", 12))
username_entry.place(x=150, y=70)
password_entry = Entry(login_window, font=("Arial", 12), show="*")
password_entry.place(x=150, y=130)
#show the password as *
password_entry.configure(show="*")

#button for login
login_button = Button(login_window, text="Login", font=("Arial", 12), bg="#2D3B40", fg="white", command=login)
login_button.place(x=150, y=200)
login_window.mainloop()


#-----------------Fota system GUI-----------------#
if flag==True:
    # CREATE A GUI FOR THE FOTA SYSTEM 
    font_setted="Times New Roman"
    button_colrs="#315959"
    fota_system_window = Tk()
    fota_system_window.title("FOTA System")
    fota_system_window.geometry("900x1000+100+50")
    fota_system_window.configure(bg="#2D3B40")
    fota_system_window.resizable()

    # Set the main label
    Label(fota_system_window, text="FOTA System", font=(font_setted, 16), bg="#2D3B40", fg="white").pack(pady=20)

    # Set the info frame - 1/3 of the main window
    info_frame = Frame(fota_system_window, bg="#2D3B40")
    info_frame.pack(pady=10, anchor="w", padx=20, fill="both", expand=False)

    # Create a canvas with a rectangle for the info section
    canvas = Canvas(info_frame, bg="#2D3B40", highlightthickness=0, width=850, height=150)
    canvas.pack(pady=10)
    canvas.create_rectangle(10, 10, 840, 140, outline="white", width=2, fill="#6C8183")

    # Add "Info Section" label inside the rectangle
    canvas.create_text(30, 25, text="Info Section", font=(font_setted, 12), fill="white", anchor="nw")

    # Create a frame inside the canvas for buttons
    button_frame = Frame(canvas, bg="#6C8183")
    canvas.create_window(425, 85, window=button_frame)

    # 1-button for bootloader version
    bootloader_version_button = Button(button_frame, text="Bootloader\nVersion", font=(font_setted, 9), 
                                   bg=button_colrs, fg="white", width=10, height=2,command=bootloader_version_handler)
    bootloader_version_button.grid(row=0, column=0, padx=0, pady=3)


    # 3-button for get chip id
    get_chip_id_button = Button(button_frame, text="Get Chip ID", font=(font_setted, 9), 
                                bg=button_colrs, fg="white", width=10, height=2,command=get_chip_id_handler)
    get_chip_id_button.grid(row=0, column=2, padx=3, pady=5)

    # 4-button for get read write protection status
    get_read_write_protection_status_button = Button(button_frame, text="Get R/W\nProtection", font=(font_setted, 9),
                     bg=button_colrs, fg="white", width=10, height=2,command=get_read_write_protection_status_handler)
    get_read_write_protection_status_button.grid(row=0, column=3, padx=3, pady=5)

    # 5-for reading option bytes
    read_option_bytes_button = Button(button_frame, text="Read Option\nBytes", font=(font_setted, 9), 
                        bg=button_colrs, fg="white", width=10, height=2,command=read_option_bytes_handler)
    read_option_bytes_button.grid(row=0, column=4, padx=3, pady=5)


    # Set the operations frame - 2/3 of the main window
    operations_frame = Frame(fota_system_window, bg="#2D3B40")
    operations_frame.pack(pady=10, padx=20, fill="both", expand=True)

    # Create two columns for erase and upload sections
    erase_frame = Frame(operations_frame, bg="#2D3B40")
    erase_frame.pack(side="left", fill="both", expand=True, padx=(0, 10))

    upload_frame = Frame(operations_frame, bg="#2D3B40")
    upload_frame.pack(side="right", fill="both", expand=True, padx=(10, 0))

    # Erase Section
    erase_canvas = Canvas(erase_frame, bg="#2D3B40", highlightthickness=0)
    erase_canvas.pack(fill="both", expand=True)
    erase_canvas.create_rectangle(10, 10, 390, 200, outline="white", width=2, fill="#6C8183")
    erase_canvas.create_text(30, 25, text="Erase Section", font=("Arial", 12), fill="white", anchor="nw")

    # Erase Section - Text boxes and buttons
    erase_button_frame = Frame(erase_canvas, bg="#6C8183")
    erase_canvas.create_window(200, 110, window=erase_button_frame)

    # Sector number label and spinbox
    Label(erase_button_frame, text="Sector Number:", font=("Arial", 10), bg="#6C8183").grid(row=0, column=0, padx=5, pady=5)
    sector_number_spinbox = Spinbox(erase_button_frame, from_=min_sector_number, to=max_sector_number, font=("Arial", 10), width=5)
    sector_number_spinbox.grid(row=0, column=1, padx=5, pady=5)

    # Number of sectors label and spinbox
    Label(erase_button_frame, text="Number of Sectors:", font=("Arial", 10), bg="#6C8183").grid(row=0, column=2, padx=5, pady=5)
    number_of_sectors_spinbox = Spinbox(erase_button_frame, from_=1, to=4, font=("Arial", 10), width=5)
    number_of_sectors_spinbox.grid(row=0, column=3, padx=5, pady=5)

    # Erase button
    erase_button = Button(erase_button_frame, text="Erase", font=("Arial", 10), 
                          bg=button_colrs, fg="white", width=10, height=1,command=erase_button_handler)
    erase_button.grid(row=1, column=0, columnspan=2, padx=5, pady=10)

    # Erase All button
    erase_all_button = Button(erase_button_frame, text="Erase All", font=("Arial", 10), 
                              bg=button_colrs, fg="white", width=10, height=1,command=erase_all_button_handler)
    erase_all_button.grid(row=1, column=2, columnspan=2, padx=5, pady=10)


    # Upload Section
    upload_canvas = Canvas(upload_frame, bg="#2D3B40", highlightthickness=0)
    upload_canvas.pack(fill="both", expand=True)
    upload_canvas.create_rectangle(10, 10, 390, 200, outline="white", width=2, fill="#6C8183")
    upload_canvas.create_text(30, 25, text="Upload Section", font=("Arial", 12), fill="white", anchor="nw")

    # Upload Section - Text box and browse button
    upload_file_frame = Frame(upload_canvas, bg="#6C8183")
    upload_canvas.create_window(200, 80, window=upload_file_frame)

    Label(upload_file_frame, text="File Path:", font=("Arial", 10), bg="#6C8183").grid(row=0, column=0, padx=5, pady=5)
    file_path_entry = Entry(upload_file_frame, font=("Arial", 10), width=20)
    file_path_entry.grid(row=0, column=1, padx=5, pady=5)

    browse_button = Button(upload_file_frame, text="Browse", font=("Arial", 10)
                           , bg=button_colrs, fg="white", width=10,command=browse_button_handler)
    browse_button.grid(row=0, column=2, padx=5, pady=5)

    # Upload/Flash button below
    upload_flash_frame = Frame(upload_canvas, bg="#6C8183")
    upload_canvas.create_window(200, 140, window=upload_flash_frame)

    upload_flash_button = Button(
        upload_flash_frame,
        text="Flash",
        font=("Arial", 10),
        bg=button_colrs,
        fg="white",
        width=15,
        height=2,
        command=upload_flash_button_handler
    )
    upload_flash_button.grid(row=0, column=0, padx=5, pady=10)

    reset_button = Button(
        upload_flash_frame,
        text="Reset",
        font=("Arial", 10),
        bg=button_colrs,
        fg="white",
        width=15,
        height=2,
        command=reset_bootloader_button_handler
    )
    reset_button.grid(row=0, column=1, padx=5, pady=10)



    # Log Section
    log_frame = Frame(fota_system_window, bg="#2D3B40")
    log_frame.pack(pady=10, padx=20, fill="both", expand=True)
    # Create a canvas with a rectangle for the log section
    log_canvas = Canvas(log_frame, bg="#2D3B40", highlightthickness=0)
    log_canvas.pack(fill="both", expand=True)
    log_canvas.create_rectangle(10, 10, 840, 350, outline="white", width=2, fill="#000000")
    log_canvas.create_text(30, 25, text="Log Section", font=(font_setted, 14, "bold"), fill="white", anchor="nw")
    # Create a text widget for logs inside the canvas
    log_text_frame = Frame(log_canvas, bg="#000000")
    log_canvas.create_window(425, 160, window=log_text_frame, width=800, height=200)
    log_text = Text(log_text_frame, font=("Arial", 9), bg="#000000", fg="#00FF00", height=15, width=95)
    log_text.pack(fill="both", expand=True)
    # Add a scrollbar to the log text
    scrollbar = Scrollbar(log_text_frame, command=log_text.yview)
    scrollbar.pack(side="right", fill="y")
    log_text.config(yscrollcommand=scrollbar.set)
    fota_system_window.mainloop()



