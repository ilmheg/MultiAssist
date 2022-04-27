import argparse
import struct
import tempfile
import subprocess
import os
import sys
import dearpygui.dearpygui as dpg
import webbrowser
import configparser
import errno
from tkinter import filedialog as fd
gui = 0

debug = False
def dbgprint(text: str) -> None:
    if debug:
        print(f"[dbg] {text}")

def define_parser():
    parser = argparse.ArgumentParser(description="Utilities for the manual animation modding workflow in XIV.",
                                    formatter_class=argparse.RawDescriptionHelpFormatter)

    subparsers = parser.add_subparsers(title="Requires (one of)", dest="command")

    extract_parser = subparsers.add_parser("extract", help="Extract a editable animation file, using a pap and sklb file from XIV.")
    extract_parser.add_argument("-s", "--skeleton-file", type=str, help="The input skeleton sklb file.", required=True)
    extract_parser.add_argument("-p", "--pap-file", type=str, help="The input animation pap file.", required=True)
    extract_parser.add_argument("-i", "--anim-index", type=str, help="The index of the animation you are extracting.", required=True)
    extract_parser.add_argument("-o", "--out-file", type=str, help="The output file.", required=True)
    extract_parser.add_argument("-t", "--file-type", type=str, help="File type to export.", required=True)

    pack_parser = subparsers.add_parser("pack", help="Repack an existing pap file with a new animation. Requires the input skeleton.")
    pack_parser.add_argument("-s", "--skeleton-file", type=str, help="The input skeleton sklb file.", required=True)
    pack_parser.add_argument("-p", "--pap-file", type=str, help="The input pap file.", required=True)
    pack_parser.add_argument("-a", "--anim-file", type=str, help="The modified animation Havok XML packfile.", required=True)
    pack_parser.add_argument("-i", "--anim-index", type=str, help="The index of the animation you are replacing.", required=True)
    pack_parser.add_argument("-o", "--out-file", type=str, help="The output pap file.", required=True)
    pack_parser.add_argument("-b", "--blend", type=bool, help="Repack and create additive animation (0/1)", required=True)

    import textwrap
    parser.epilog = textwrap.dedent(
        f"""\
        commands usage:
        {extract_parser.format_usage()}
        {pack_parser.format_usage()}
        Remember that each command also has its own, more descriptive, -h/--help arg, describing what file types are expected.
        """
    )
    return parser

def animassist_check():
    if not os.path.exists("animassist.exe"):
            print("Animassist.exe is missing :(")
            if(gui!=0):
                    gui.show_info("Error!","Animassist.exe is missing")
            sys.exit(1)

def to_havok_check():
    if not os.path.exists("fbx2havok.exe"):
            print("fbx2havok.exe is missing :(")
            if(gui!=0):
                    gui.show_info("Error!","fbx2havok.exe is missing")
            sys.exit(1)

def to_fbx_check():
    if not os.path.exists("tofbx.exe"):
            print("tofbx.exe is missing :(")
            if(gui!=0):
                    gui.show_info("Error!","tofbx.exe is missing")
            sys.exit(1)

def assist_skl_tag(skeleton_path, out_path) -> None:
    animassist_check()

    complete = subprocess.run(["animassist.exe", "1", skeleton_path, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")
    if not os.path.exists(out_path):
        print("skeleton binary tag file operation failed")
    else:
        dbgprint(f"Saved skeleton xml to {out_path}")

def assist_skl_anim_pack(skeleton_path, out_path) -> None:
    animassist_check()

    complete = subprocess.run(["animassist.exe", "2", skeleton_path, out_path], capture_output=True, encoding="utf8",text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")
    if not os.path.exists(out_path):
        print("skeleton xml packfile assist failed")
    else:
        dbgprint(f"Saved packfile to {out_path}")

def assist_combine(skeleton_path, animation_path, animation_index, out_path) -> None:
    animassist_check()
    
    complete = subprocess.run(["animassist.exe", "3", skeleton_path, animation_path, animation_index, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("binary packfile assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def assist_xml(skl_path, animation_path, out_path) -> None:
    animassist_check()
    
    complete = subprocess.run(["animassist.exe", "4", skl_path, animation_path, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("xml packfile assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def assist_tag(xml_path, out_path) -> None:
    animassist_check()
    
    complete = subprocess.run(["animassist.exe", "5", xml_path, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("xml tagfile assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")
    return

def assist_combine_tag(skeleton_path, animation_path, animation_index, out_path):
    animassist_check()
    
    complete = subprocess.run(["animassist.exe", "6", skeleton_path, animation_path, animation_index, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("binary tagfile assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def assist_blend(animation_path, out_path):
    animassist_check()
    complete = subprocess.run(["animassist.exe", "7",  animation_path, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("additive assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def assist_repack(mod_path, animation_path, animation_index, out_path):
    animassist_check()
    complete = subprocess.run(["animassist.exe", "8", mod_path, animation_path, animation_index, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("repack assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def assist_compress(animation_path, out_path):
    animassist_check()
    complete = subprocess.run(["animassist.exe", "9", animation_path, out_path], capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.returncode}")
    dbgprint(f"{complete.stdout}")

    if not os.path.exists(out_path):
        print("animation compression assist failed")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def to_hkx(skeleton_path, hkx_path, fbx_path, out_path) -> None:
    to_havok_check()
    complete = subprocess.run("fbx2havok.exe -hk_skeleton \"" + skeleton_path + "\" -hk_anim \"" + hkx_path + "\" -fbx " + fbx_path + " -hkout \"" + out_path +"\"", capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    print(f"{complete.stdout}")
    print(f"{complete.returncode}")
    #print(f"{complete.stderr}")
    if not os.path.exists(out_path):
        print("fbx2havok operation failed.")
    else:
        dbgprint(f"Saved importable file to {out_path}")

def to_fbx(skeleton_path, hkx_path, out_path):
    to_fbx_check()
    complete = subprocess.run("tofbx.exe -hk_skeleton \"" + skeleton_path + "\" -hk_anim \"" + hkx_path + "\" -fbx \"" + out_path +"\"", capture_output=True, encoding="utf8", creationflags=subprocess.CREATE_NO_WINDOW)
    dbgprint(f"{complete.stdout}")
    dbgprint(f"{complete.returncode}")
    if not os.path.exists(out_path):
        print("havok2fbx operation failed.")
    else:
        print(f"Saved importable file to {out_path}")
       
def export(skeleton_path, pap_path, anim_index, output_path, file_type):

    with open(pap_path, "rb") as p:
        pap_data = p.read()
    with open(skeleton_path, "rb") as s:
        sklb_data = s.read()

    sklb_hdr = read_sklb_header(sklb_data)
    pap_hdr = read_pap_header(pap_data)
    
    print(f"The input skeleton is for ID {sklb_hdr['skele_id']}.")
    print(f"The input animation is for ID {pap_hdr['skele_id']}.")
    print(f"If these mismatch, things will go very badly.")
    
    num_anims = len(pap_hdr["anim_infos"])
    if num_anims > 1:
        anim_index = pap_hdr["anim_infos"][int(anim_index)]["havok_index"] # Just to be safe, unsure if this index will ever mismatch
    else:
        anim_index = 0

    with tempfile.TemporaryDirectory() as tmp_folder:
            tmp_output_bin_path = os.path.join(tmp_folder, "tmp_ouwut_bin")
            tmp_skel_path = os.path.join(tmp_folder, "tmp_skel")
            tmp_anim_path = os.path.join(tmp_folder, "tmp_anim")
            tmp_skel_xml_path = os.path.join(tmp_folder, "tmp_skel_xml")
            tmp_anim_bin_path  = os.path.join(tmp_folder, "tmp_anim_bin")
            
            dbgprint(f"we have {tmp_skel_path} as tmp_skel")
            dbgprint(f"we have {tmp_anim_path} as tmp_anim")
            havok_skel = get_havok_from_sklb(sklb_data)
            havok_anim = get_havok_from_pap(pap_data)
            with open(tmp_skel_path, "wb") as tmp_skel:
                tmp_skel.write(havok_skel)
            with open(tmp_anim_path, "wb") as tmp_anim:
                tmp_anim.write(havok_anim)
            assist_skl_tag(tmp_skel_path, tmp_skel_xml_path)
            if (file_type=="fbx"): 
                print()
                assist_combine_tag(tmp_skel_path, tmp_anim_path, str(anim_index), tmp_anim_bin_path)
                to_fbx(tmp_skel_path,tmp_anim_bin_path,output_path)
            elif (file_type=="hkxp"):
                assist_combine(tmp_skel_path, tmp_anim_path, str(anim_index), output_path)
            elif (file_type=="hkxt"):
                assist_combine_tag(tmp_skel_path, tmp_anim_path, str(anim_index), output_path)
            elif (file_type=="xml"):
                assist_xml(tmp_skel_path, tmp_anim_path, output_path)
            

def multi_repack(skeleton_file : str, anim_file : str, mod_file : str, anim_index, out_path : str, additive : bool=False, compress : bool=False):
    with open(skeleton_file, "rb") as s:
        orig_sklb_data = s.read()
    with open(anim_file, "rb") as a:
        orig_pap_data = a.read()
    with open(mod_file, "rb") as m:
        mod_data = m.read()
    
    print("a")
    sklb_hdr = read_sklb_header(orig_sklb_data)
    pap_hdr = read_pap_header(orig_pap_data)
    print(f"The input skeleton is for ID {sklb_hdr['skele_id']}.")
    print(f"The input animation is for ID {pap_hdr['skele_id']}.")
    print(f"If these mismatch, things will go very badly.")
    with tempfile.TemporaryDirectory() as tmp_folder:
        tmp_fbx_path = os.path.join(tmp_folder, "tmp_fbx_bin")    

        tmp_skel_path = os.path.join(tmp_folder, "tmp_skel_bin")   
        tmp_pap_path = os.path.join(tmp_folder, "tmp_pap_bin")     
        tmp_mod_path = os.path.join(tmp_folder, "tmp_mod_bin")    

        tmp_out_bin_path = os.path.join(tmp_folder, "tmp_out_bin") 

        havok_skel = get_havok_from_sklb(orig_sklb_data)
        havok_pap = get_havok_from_pap(orig_pap_data)
        
        with open(tmp_fbx_path, "wb") as tmp_fbx:
            tmp_fbx.write(mod_data)
        with open(tmp_skel_path, "wb") as tmp_skel:
            tmp_skel.write(havok_skel)
        with open(tmp_pap_path, "wb") as tmp_pap:
            tmp_pap.write(havok_pap)
        
        if(mod_file.endswith(".fbx")):
            dbgprint("File Size A: " + str(os.path.getsize(tmp_skel_path)))
            dbgprint("File Size B: " + str(os.path.getsize(tmp_pap_path)))
            dbgprint("File Size C: " + str(os.path.getsize(tmp_fbx_path)))

            to_hkx(tmp_skel_path, tmp_pap_path, tmp_fbx_path, tmp_mod_path)
        else:
            tmp_mod_path = tmp_fbx_path

        if additive: 
            assist_blend(tmp_mod_path,tmp_mod_path)
        if compress:
            assist_compress(tmp_mod_path,tmp_mod_path)
        
        assist_repack(tmp_mod_path,tmp_pap_path,str(anim_index),tmp_out_bin_path)
        
        with open(tmp_out_bin_path, "rb") as modbin:
            new_havok = modbin.read()

        pre_havok = bytearray(orig_pap_data[:pap_hdr["havok_offset"]])
        print(len(pre_havok) + len(new_havok))
        new_timeline_offset = len(pre_havok) + len(new_havok) 
        
        offs_bytes = new_timeline_offset.to_bytes(4, "little")
        
        for i in range(4):
            pre_havok[22 + i] = offs_bytes[i]
        
        post_havok = orig_pap_data[pap_hdr["timeline_offset"]:]

        # Write .pap file
        if os.path.getsize(tmp_out_bin_path) != 0:
            with open(out_path, "wb") as out:
                out.write(pre_havok)
                out.write(new_havok)
                out.write(post_havok)
            print(f"Wrote new pap to {out_path}!")
            
SKLB_HDR_1 = ['magic', 'version', 'offset1', 'havok_offset', 'skele_id', 'other_ids']
def read_sklb_header_1(sklb_data) -> dict:
    # 4 byte magic
    # 4 byte version
    # 2 byte offset to ?
    # 2 byte offset to havok
    # 4 byte skeleton id
    # 4 x 4 byte other skeleton ids

    hdr = { k: v for k, v in zip(SKLB_HDR_1, struct.unpack('<4sIHHI4I', sklb_data[0:32])) }
    for key in hdr.keys():
        dbgprint(f"{key} : {hdr[key]}")
    return hdr

SKLB_HDR_2 = ['magic', 'version', 'offset1', 'havok_offset', 'unknown1', 'skele_id', 'other_ids']
def read_sklb_header_2(sklb_data) -> dict:
    # 4 byte magic
    # 4 byte version
    # 4 byte offset to ?
    # 4 byte offset to havok
    # 4 byte ?
    # 4 byte skeleton id
    # 4 x 4 byte other skeleton ids

    hdr = { k: v for k, v in zip(SKLB_HDR_2, struct.unpack('<4sIIIII4I', sklb_data[0:40])) }
    for key in hdr.keys():
        dbgprint(f"{key} : {hdr[key]}")
    return hdr

def read_sklb_header(sklb_data) -> dict:
    magic, ver = struct.unpack("<II", sklb_data[0:8])
    if ver != 0x31333030:
        hdr = read_sklb_header_1(sklb_data)
    else:
        hdr = read_sklb_header_2(sklb_data)
    return hdr

def get_havok_from_sklb(sklb_data):
    hdr = read_sklb_header(sklb_data)
    dbgprint(f"This skeleton file is for the skeleton c{hdr['skele_id']}.")

    havok_start = hdr['havok_offset']
    havok_end = len(sklb_data)
    new_data = sklb_data[havok_start:havok_end]

    return new_data

PAP_HDR = ['magic', 'version', 'anim_count', 'skele_id', 'info_offset', 'havok_offset', 'timeline_offset']
ANIM_INFO = ['name', 'unknown1', 'havok_index', 'unknown2']
def read_pap_header(pap_data) -> dict:
    # 4 byte magic 4
    # uint version 8
    # ushort info num 10
    # uint skele id 14
    # uint offset to info 18
    # uint offset to havok container 22
    # uint offset to timeline 26

    hdr = { k: v for k, v in zip(PAP_HDR, struct.unpack('<4sIHIIII', pap_data[0:26])) }

    hdr["anim_infos"] = []
    for i in range(hdr["anim_count"]):
        start = 26 + 40 * i
        end = 26 + 40 * (i + 1)
        anim_info = { k: v for k, v in zip(ANIM_INFO, struct.unpack('<32sHHI', pap_data[start:end])) }
        anim_info["name"] = str(anim_info["name"], encoding="utf8").split("\0")[0]
        hdr["anim_infos"].append(anim_info)

    for key in hdr.keys():
        dbgprint(f"{key} : {hdr[key]}")

    return hdr

def get_havok_from_pap(pap_data):
    hdr = read_pap_header(pap_data)
    dbgprint(f"This pap file is for the skeleton c{hdr['skele_id']}. It has {hdr['anim_count']} animation(s).")

    havok_start = hdr['havok_offset']
    havok_end = hdr['timeline_offset']
    new_data = pap_data[havok_start:havok_end]

    return new_data

def read_timeline(tmb_data):
    # 4 bytes TMLB
    # UInt size
    # UInt entries 

    # 4 bytes TMDH
    # UInt unk
    # UShort ID
    # UShort unk
    # UShort unk
    # UShort unk

    # 4 bytes TMAL OR TMPP
    # if PP:
    #   UInt unk
    #   UInt offset
    #   4 bytes TMAL
    # UInt unk
    # UInt offset
    # UInt number of TMAC

    # TMAC
    # UInt unk
    # UShort ID
    # UShort Time
    # UInt Unk
    # UInt Unk
    # UInt offset
    # UInt number of TMTR

    # TMTR
    # UInt size
    # UShort ID
    # UShort Time
    # UInt Offset
    # Count of CXXX per track
    # UInt Unk offset
    #   unk reaper stuff

    # CXXX
    # Depends on XXX /pdead

    # name of animation:
    #   name of expression 
    return

class GUI:
    EXPORT_TYPES = ['.fbx', '.hkx packfile (HavokMax compatible)', '.hkx tagfile', '.xml']
    EXPORT_EXT = ['.fbx', '.hkx', '.hkx', '.xml']
    def __init__(self) -> None:
        self.config = configparser.ConfigParser()
        self._config()
        self.anims=[]
        self.reanims=[]
        self._run()

        
    def start_loading(self):
        dpg.configure_item("loading_export", show=True)
        dpg.configure_item("loading_repack", show=True)
        return
    
    def stop_loading(self):
        dpg.configure_item("loading_export", show=False)
        dpg.configure_item("loading_repack", show=False)
        return

    def show_info(self, title, message):
        # guarantee these commands happen in the same frame
        with dpg.mutex():

            viewport_width = dpg.get_viewport_client_width()
            viewport_height = dpg.get_viewport_client_height()

            with dpg.window(label=title, modal=True, no_close=True) as modal_id:
                dpg.add_text(message)
                dpg.add_button(label="Ok", width=75, user_data=(modal_id, True), callback=lambda:dpg.delete_item(modal_id))

        # guarantee these commands happen in another frame
        dpg.split_frame()
        width = dpg.get_item_width(modal_id)
        height = dpg.get_item_height(modal_id)
        dpg.set_item_pos(modal_id, [viewport_width // 2 - width // 2, viewport_height // 2 - height // 2])

    def _config(self):
        if not os.path.exists('config.ini'):
            self.config['SETTINGS'] = {'export_iteration': 'false', 'export_location': os.path.abspath(os.curdir), 'export_type': '0'}

            self.config.write(open('config.ini', 'w'))

    def _get_config(self, section, key, bool=False):
        self.config.read('config.ini')
        try:
            if bool: return self.config.getboolean(section, key)
            return self.config.get(section, key)
        except configparser.NoOptionError or configparser.NoSectionError:
            # TODO: prompt user to regenerate config
            if bool: return False
            return ""

    def _save_settings(self):
        self.config.set('SETTINGS','export_iteration', str(not dpg.get_value("settings_export_iteration")))
        self.config.set('SETTINGS','export_location', str(dpg.get_value("settings_export_location")))
        self.config.set('SETTINGS','export_type', str(self.EXPORT_TYPES.index(dpg.get_value("settings_export_type"))))
        self.config.write(open('config.ini', 'w'))          

    def _get_file(self, file_types, output_tag):
        file = fd.askopenfilename(filetypes=file_types)
        if file!= "": dpg.set_value(output_tag, file)

    def _get_folder(self,output_tag):
        folder = fd.askdirectory()
        if folder!= "": dpg.set_value(output_tag, folder)


    def _populate_anims(self, sender, app_data, user_data):
        if dpg.get_value(user_data['pap_input']) == "" or dpg.get_value(user_data['sklb_input']) == "":
            self.show_info("Error", "Please make sure you have selected both a .pap and .sklb file.")
            return
        if not os.path.exists(dpg.get_value(user_data['pap_input'])) or not os.path.exists(dpg.get_value(user_data['sklb_input'])):
            self.show_info("Error","One or more of the files selected does not exist.")
            return

        with open(dpg.get_value(user_data['pap_input']), "rb") as p:
            pap_data = p.read()
        with open(dpg.get_value(user_data['sklb_input']), "rb") as s:
            sklb_data = s.read()

        pap_hdr = read_pap_header(pap_data)
        sklb_hdr = read_sklb_header(sklb_data)
        num_anims = len(pap_hdr["anim_infos"])

        if(sklb_hdr['skele_id']!=pap_hdr['skele_id']):
            self.show_info("Warning!", "The Skeleton ID ("+str(sklb_hdr['skele_id'])+") does not match the Skeleton ID in the .pap ("+ str(pap_hdr['skele_id']) +")\n\nAre you sure you are importing the right files?")

        a = []
        for i in range(num_anims):
            a.append(pap_hdr['anim_infos'][i]['name'])
        dpg.configure_item(user_data['output'], default_value=a[0])
        dpg.configure_item(user_data['output'], items=a)
        if(user_data['output'] == "anim_list"):
            self.anims = a
        if(user_data['output'] == "reanim_list"):
            self.reanims = a

    def _clear_anims(self, sender, app_data, user_data):
        if(user_data['output'] == []):
            if(self.anims == []): return
        if(user_data['output'] == "reanim_list"):
            if(self.reanims == []): return

        dpg.configure_item(user_data['output'], default_value=0)
        dpg.configure_item(user_data['output'], items=[])
        if(user_data['output'] == "anim_list"):
            self.anims = []
        if(user_data['output'] == "reanim_list"):
            self.reanims = []
        return

    def _get_ft(self):
        # kinda dislike this
        x = dpg.get_value("extension_selector")
        if (x == self.EXPORT_TYPES[0]):
            return "fbx"
        if (x == self.EXPORT_TYPES[1]):
            return "hkxp"
        if (x == self.EXPORT_TYPES[2]):
            return "hkxt"
        if (x == self.EXPORT_TYPES[3]):
            return "xml"

    def _copy_tab(self, tab):
        if tab == "repack":
            if dpg.get_value("selected_repap") != "": dpg.set_value("selected_pap", dpg.get_value("selected_repap"))
            if dpg.get_value("selected_resklb") != "": dpg.set_value("selected_sklb", dpg.get_value("selected_resklb"))
        elif tab == "extract":
            if dpg.get_value("selected_pap") != "": dpg.set_value("selected_repap", dpg.get_value("selected_pap"))
            if dpg.get_value("selected_sklb") != "": dpg.set_value("selected_resklb", dpg.get_value("selected_sklb"))

    def _export_window(self):
        with dpg.child_window(autosize_x=True, height=200):
            with dpg.group():
                dpg.add_text("Select .pap:")
                with dpg.group(horizontal=True):
                    #dpg.add_text("None selected", tag="selected_pap_status")
                    dpg.add_input_text(tag="selected_pap", callback=self._clear_anims, user_data={"output":"anim_list"})
                    dpg.add_button(label="...", callback=lambda: self._get_file([("XIV animation file", "*.pap")], "selected_pap"))
                dpg.add_text("Select .sklb:")
                with dpg.group(horizontal=True):
                    #dpg.add_text("None selected", tag="selected_sklb_status")
                    dpg.add_input_text(label="", tag="selected_sklb")
                    dpg.add_button(label="...", callback=lambda: self._get_file([("XIV skeleton file", "*.sklb")], "selected_sklb"))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Submit", callback=self._populate_anims, user_data={"output" : "anim_list", "pap_input":"selected_pap", "sklb_input":"selected_sklb"})
                    dpg.add_button(label="Copy Repack Tab", callback=lambda: self._copy_tab("repack"))
        with dpg.group(horizontal=True):
            with dpg.child_window(autosize_y=True, width=200):
                with dpg.group():
                    dpg.add_text("Select Animation")
                    dpg.add_radio_button(tag="anim_list", label="radio")
            with dpg.child_window(autosize_y=True, autosize_x=True, tag="export_window"):
                dpg.add_text("Export Options")
                dpg.add_text("Export as: ")
                if self._get_config('SETTINGS','export_type') == "":
                    e = self.EXPORT_TYPES[0]
                else:
                    e = self.EXPORT_TYPES[int(self._get_config('SETTINGS','export_type'))]
                with dpg.group(horizontal=True):
                    dpg.add_combo(items=self.EXPORT_TYPES, default_value=e, tag="extension_selector", callback=lambda: dpg.set_value("extension", dpg.get_value("extension_selector")[0:4]))
                dpg.add_text("Name: ")
                with dpg.group(horizontal=True):                    
                    dpg.add_input_text(tag="name")
                    dpg.add_text(self.EXPORT_EXT[int(self.EXPORT_TYPES.index(e))], tag="extension")
                dpg.add_text("Export directory: ")
                with dpg.group(horizontal=True):
                    dpg.add_input_text(label="", tag="export_directory", default_value=self._get_config('SETTINGS', 'export_location'))
                    dpg.add_button(label="...", callback=lambda: self._get_folder("export_directory"))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Export", callback=self._export_callback) 
                    dpg.add_loading_indicator(color=(169, 127, 156),secondary_color=(66, 49, 61),style=1, radius=1.2, show=False, tag="loading_export")
    
    def _success_check(self, path):
        if not os.path.exists(path):
            dbgprint("No file was written")
            self.show_info("Error!","No file was written, something went wrong.\n\nPlease ensure that input files are correct and you have\n permission to write to this export directory.")
        elif os.path.getsize(path == 0):
            self.show_info("Error!","A file was written but it is empty.\n\nPlease ensure input files are correct.")
        else:
            dbgprint(f"Saved to {path}")
            self.show_info("Success!","You can find your exported file at:\n" + path)

    def _file_iteration(self, path):
        split = os.path.splitext(path)
        file_count = 1
        while os.path.exists(split[0]+"_("+str(file_count)+")"+split[1]):
            file_count+=1
        return split[0]+"_("+str(file_count)+")"+split[1]

    def _export_callback(self, sender, app_data):
        if not os.path.exists(dpg.get_value("selected_pap")) or not os.path.exists(dpg.get_value("selected_sklb")):
            self.show_info("Error","One or more of the files selected does not exist.")
            return
            
        path = dpg.get_value("export_directory")+"/"+dpg.get_value("name")+dpg.get_value("extension")
        if os.path.exists(path) and self._get_config('SETTINGS', 'export_iteration', True):
                path = self._file_iteration(path)

        #if not os.access(path, os.W_OK) or not os.access( path, os.X_OK):
        #    self.show_info("Error", "You do not have permission to write to this export directory")
        #    return
        if self.anims == []:
            self.show_info("Error","No animation selected. Did you press [Submit]?")
            return

        self.start_loading()
        try:
            
            export(dpg.get_value("selected_sklb"), dpg.get_value("selected_pap"), self.anims.index(dpg.get_value("anim_list")), path,self._get_ft())
            self._success_check(path)
        except Exception as e:
            print(e)
            self.show_info("Error", "An error occurred within the extraction process. No file was written.\n\n"+repr(e))
        self.stop_loading()
         
    def _repack_callback(self, sender, app_data):
        if not os.path.exists(dpg.get_value("selected_repap")) or not os.path.exists(dpg.get_value("selected_resklb"))  or not os.path.exists(dpg.get_value("selected_fbx")):
            self.show_info("Error","One or more of the files selected does not exist.")
            return
        if self.reanims == []:
            self.show_info("Error","No animation selected. Did you press [Submit]?")
            return
        
        path = dpg.get_value("reexport_directory")+"/"+dpg.get_value("rename")+".pap"
        if os.path.exists(path) and self._get_config('SETTINGS', 'export_iteration', True):
            path = self._file_iteration(path)

        self.start_loading()
        try:
            
            multi_repack(dpg.get_value("selected_resklb"),
                        dpg.get_value("selected_repap"),
                        dpg.get_value("selected_fbx"), 
                        self.reanims.index(dpg.get_value("reanim_list")), 
                        path,  
                        dpg.get_value("additive_setting"),
                        dpg.get_value("compress_setting"))
            self._success_check(path)
        except IOError as e:
            print(e)
            if(e.errno == errno.EPERM or e.errno == errno.EACCES):
                self.show_info("Error", "You do not have permission to write to that directory. No file was written.")
            elif(e.errno == errno.EADDRINUSE):
                self.show_info("Error", "The path you are writing to is in use. No file was written.")
            else:
                self.show_info("Error", "An IOError occured:\n\n"+e.strerror)
        except Exception as err:
            print(err)
            self.show_info("Error", "An error occurred within the extraction process. No file was written.\n\n"+repr(err))

        self.stop_loading()

    def _repack_window(self):
        with dpg.child_window(autosize_x=True, height=200):
            dpg.add_text("Select .pap:")
            with dpg.group(horizontal=True):
                dpg.add_input_text(tag="selected_repap", callback=self._clear_anims, user_data={"output":"reanim_list"})
                dpg.add_button(label="...", callback=lambda: self._get_file([("XIV animation file", "*.pap")], "selected_repap"))
            dpg.add_text("Select .sklb:")
            with dpg.group(horizontal=True):
                dpg.add_input_text(tag="selected_resklb")
                dpg.add_button(label="...", callback=lambda: self._get_file([("XIV skeleton file", "*.sklb")], "selected_resklb"))
            dpg.add_text("Select .fbx or HavokMax .hka/.hkx/.hkt:")
            with dpg.group(horizontal=True):
                dpg.add_input_text(tag="selected_fbx")
                dpg.add_button(label="...", callback=lambda: self._get_file([("Edited animation", "*.fbx *.hkx *.hka *.hkt")], "selected_fbx"))
            with dpg.group(horizontal=True):
                dpg.add_button(label="Submit", callback=self._populate_anims, user_data={"output" : "reanim_list", "pap_input":"selected_repap", "sklb_input":"selected_resklb"})
                dpg.add_button(label="Copy Extract Tab", callback=lambda: self._copy_tab("extract"))
        with dpg.group(horizontal=True):
            with dpg.child_window(autosize_y=True, width=200):
                dpg.add_text("Select Animation")
                dpg.add_radio_button(tag="reanim_list", label="radio", default_value=0)
            with dpg.child_window(autosize_y=True, autosize_x=True, tag="reexport_window"):
                dpg.add_text("Repack Options")
                with dpg.group(horizontal=True):   
                    dpg.add_checkbox(label="Compress", tag="compress_setting") 
                    dpg.add_button(label="?", callback=lambda:self.show_info("Info","Compression provides file size reduction at the cost of some detail. \nMultiAssist uses predictive compression whereas XIV animations are spline compressed. \nSpline compression animations often don't function as expected."))
                    dpg.add_checkbox(label="Additive (PROTOTYPE)", tag="additive_setting")
                    dpg.add_button(label="?", callback=lambda:self.show_info("Info","Additive animation will not function with non-edited hkx animations. \nFurthermore, the implementation is different to that within XIV. \nRe-importing edited additive animations will also likely not function as expected."))
                dpg.add_text("Name:")
                with dpg.group(horizontal=True):                    
                    dpg.add_input_text(tag="rename")
                    dpg.add_text(".pap")
                dpg.add_text("Export directory: ")
                with dpg.group(horizontal=True):
                    dpg.add_input_text(label="", tag="reexport_directory", default_value=self._get_config('SETTINGS', 'export_location'))
                    dpg.add_button(label="...", callback=lambda: self._get_folder("reexport_directory"))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Repack", callback=self._repack_callback)
                    dpg.add_loading_indicator(color=(169, 127, 156),secondary_color=(66, 49, 61),style=1, radius=1.2, show=False, tag="loading_repack")

    def _skeleton_window(self):
        with dpg.child_window(autosize_x=True):
            with dpg.group(horizontal=True):
                dpg.add_radio_button(horizontal=True, items=["Export .sklb to FBX", "Generate .sklb from FBX"])
        
            
            dpg.add_text("Select .sklb")
            with dpg.group(horizontal=True):
                dpg.add_input_text()
                dpg.add_button(label="...", callback=lambda: self._get_file([("XIV skeleton file", "*.sklb")], "selected_resklb"))
            
            dpg.add_text("Name")
            with dpg.group(horizontal=True):
                dpg.add_input_text()
            dpg.add_text("Destination Folder")
            with dpg.group(horizontal=True):
                dpg.add_input_text()
                dpg.add_button(label="...", callback=lambda: self._get_file([("XIV skeleton file", "*.sklb")], "selected_resklb"))

    def _set_theme(self):
        accent_light = (239, 179, 221)
        accent = (206, 154, 190)
        accent_dark = (169, 127, 156)
        with dpg.theme() as global_theme:

            with dpg.theme_component(dpg.mvAll):
                dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (59, 44, 55), category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_FrameBgHovered, accent_light, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_FrameBgActive, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_TitleBgActive, accent_dark, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_CheckMark, accent_dark, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_SliderGrab, accent_light, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_SliderGrabActive, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, accent_light, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_HeaderHovered, accent_light, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_HeaderActive, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_TabHovered, accent_light, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_TabActive, accent, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_TabUnfocusedActive, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_DockingPreview, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvThemeCol_TextSelectedBg, accent_dark, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvNodeCol_TitleBarHovered, accent_dark, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvNodeCol_TitleBarSelected, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvNodeCol_LinkHovered, accent, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvNodeCol_LinkSelected, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_color(dpg.mvNodeCol_BoxSelector, accent, category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvNodeCol_BoxSelectorOutline, accent, category=dpg.mvThemeCat_Core)

                dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 5, category=dpg.mvThemeCat_Core)

        dpg.bind_theme(global_theme)
        
    def _run(self):
        dpg.create_context()
        
        with dpg.window(tag="Primary Window"):
            with dpg.menu_bar():
                with dpg.menu(label="Settings"):
                    if self._get_config('SETTINGS','export_type') == "":
                        export_type = self.EXPORT_TYPES[0]
                    else:
                        export_type = self.EXPORT_TYPES[int(self._get_config('SETTINGS','export_type'))]
                        
                    dpg.add_checkbox(label="Overwrite export if file exists", default_value= not self._get_config('SETTINGS','export_iteration', True), tag="settings_export_iteration")
                    dpg.add_text("\nDefault extract type:")
                    
                    dpg.add_combo(default_value=export_type , tag="settings_export_type", items=self.EXPORT_TYPES)
                    dpg.add_text("\nDefault export directory:")
                    with dpg.group(horizontal=True):
                        dpg.add_input_text(default_value=self._get_config('SETTINGS','export_location'), tag="settings_export_location")
                        dpg.add_button(label="...", callback=lambda: self._get_folder("settings_export_location"))
                        
                    dpg.add_button(label="Save settings", callback=lambda:self._save_settings())
                dpg.add_menu_item(label="Wiki", callback=lambda:webbrowser.open('https://github.com/ilmheg/MultiAssist/wiki'))
                with dpg.menu(label="Useful Tools & Alternatives"):
                    dpg.add_text("Animation Editing")
                    dpg.add_button(label="0ceal0t's BlenderAssist", callback=lambda:webbrowser.open('https://github.com/0ceal0t/BlenderAssist/tree/main/BlenderAssist'))
                    dpg.add_button(label="0ceal0t's VFXEditor", callback=lambda:webbrowser.open('https://github.com/0ceal0t/Dalamud-VFXEditor'))
                    dpg.add_text("File Extraction & Importing")
                    dpg.add_button(label="FFXIV Explorer", callback=lambda:webbrowser.open('https://github.com/goaaats/ffxiv-explorer-fork'))
                    dpg.add_button(label="TexTools", callback=lambda:webbrowser.open('https://www.ffxiv-textools.net/'))
                    dpg.add_button(label="Penumbra", callback=lambda:webbrowser.open('https://github.com/xivdev/Penumbra/'))
                dpg.add_menu_item(label="                       ", enabled=False) # one right align pls
                dpg.add_menu_item(label="Report an Issue!", callback=lambda:webbrowser.open('https://github.com/ilmheg/MultiAssist/issues'))
            with dpg.group(horizontal=True):
                dpg.add_text("MultiAssist")
            
            with dpg.tab_bar(label='tabbar'):  
                with dpg.tab(label='Extract'):  
                    self._export_window()
                with dpg.tab(label='Repack'):   
                    self._repack_window()
                #with dpg.tab(label='Skeleton Tools'): 
                #     self._skeleton_window()  

        self._set_theme()

        dpg.create_viewport(title='MultiAssist', width=650, height=520, min_width=500,min_height=520)
        #dpg.set_viewport_large_icon("icon/icon_large.ico")
        #dpg.set_viewport_small_icon("icon/icon_large.ico")

        dpg.setup_dearpygui()
        dpg.show_viewport()
        dpg.set_primary_window("Primary Window", True)
        dpg.start_dearpygui()
        dpg.destroy_context()

def main():
    parser = define_parser()

    try:
        args = parser.parse_args()
    except FileNotFoundError:
        print("All input files must exist!")

    if args.command == "extract":
        export(args.skeleton_file, args.pap_file, args.anim_index, args.out_file, args.file_type)
    elif args.command == "pack":
        multi_repack(args.skeleton_file, args.pap_file, args.anim_file, args.anim_index, args.out_file, args.blend)
    else:
        global gui
        gui = GUI()

if __name__ == "__main__":
    main()