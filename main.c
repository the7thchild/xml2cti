#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024

/* Helper to safely strip quotes from XML attributes */
void get_attr_value(const char *line, const char *attr, char *dest) {
    char *p = strstr(line, attr);
    dest[0] = '\0';
    if (p) {
        p = strchr(p, '"');
        if (p) {
            char *end = strchr(p + 1, '"');
            if (end) {
                int len = end - (p + 1);
                strncpy(dest, p + 1, len);
                dest[len] = '\0';
            }
        }
    }
}

int main(int argc, char **argv) {
    FILE *in, *out;
    char line[MAX_LINE];
    char val[MAX_LINE];
	int No_Audio_Track;
    
    /* Metadata buffers extracted from the XML */
    char vol_id[128] = "PLAYSTATION";
    char sys_id[128] = "PLAYSTATION";
    char publisher[128] = "HUDSON";
    char preparer[128] = "HUDSON";
    
    int indent = 8; /* Starting indentation for structural hierarchy */
    int i;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.xml> <output.cti>\n", argv[0]);
        return 1;
    }

    in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "Error opening input file: %s\n", argv[1]);
        return 1;
    }

    out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "Error opening output file: %s\n", argv[2]);
        fclose(in);
        return 1;
    }

    /* Pass 1: Parse Global Identifiers */
    while (fgets(line, sizeof(line), in)) {
        if (strstr(line, "<identifiers")) {
            get_attr_value(line, "volume=", vol_id);
            get_attr_value(line, "system=", sys_id);
            get_attr_value(line, "publisher=", publisher);
            get_attr_value(line, "data_preparer=", preparer);
            break;
        }
    }
    fseek(in, 0, SEEK_SET);

    /* Write boilerplate CTI schema header block */
    fprintf(out, "Disc CDROMXA_PSX\n\n");
    fprintf(out, "LeadIn XA\n  Empty 1350\n  PostGap 150\nEndTrack\n\n");
    fprintf(out, "Track XA\n  Pause 150\n  Volume ISO9660\n");
    fprintf(out, "    SystemArea C:\\CDGEN\\LICENSEJ.DAT\n");
    fprintf(out, "    PrimaryVolume\n");
    fprintf(out, "      SystemIdentifier %s\n", sys_id);
    if (vol_id && vol_id[0] != '\0') {fprintf(out, "      VolumeIdentifier %s\n", vol_id);}
    if (publisher && publisher[0] != '\0') {fprintf(out, "      PublisherIdentifier %s\n", publisher);}
    if (preparer && preparer[0] != '\0') {fprintf(out, "      DataPreparerIdentifier %s\n", preparer);}
    fprintf(out, "      ApplicationIdentifier PLAYSTATION\n");
    fprintf(out, "      LPath\n      MPath\n      Hierarchy\n");

    /* Pass 2: Main File and Directory Hierarchy Processing */
    while (fgets(line, sizeof(line), in)) {
        
        /* Directory tags matching pattern <dir name="..." source="..."> */
        if (strstr(line, "<dir ") && !strstr(line, "/>")) {
            get_attr_value(line, "name=", val);
            for (i = 0; i < indent; i++) fputc(' ', out);
            fprintf(out, "Directory %s\n", val);
            indent += 2;
        } 
        /* Closing Directory structures */
        else if (strstr(line, "</dir>")) {
            indent -= 2;
            for (i = 0; i < indent; i++) fputc(' ', out);
            fprintf(out, "EndDirectory\n");
        }
        /* File Entry Processing */
        else if (strstr(line, "<file ")) {
            char name[256], source[256], type[64];
            
            get_attr_value(line, "name=", name);
            get_attr_value(line, "source=", source);
            get_attr_value(line, "type=", type);

            /* Skip CD-DA track references if they are inside the data XML tree layout */
            if (strcmp(type, "da") == 0) {
                continue;
            }

            /* Convert source path Unix slashes to Windows layout */
            for (i = 0; source[i] != '\0'; i++) {
                if (source[i] == '/') source[i] = '\\';
            }

            for (i = 0; i < indent; i++) fputc(' ', out);
            fprintf(out, "File %s\n", name);

            /* Differentiate between Standard Data Form1 and Mixed Mode XA/STR streams */
            if (strcmp(type, "mixed") == 0) {
                for (i = 0; i < indent + 2; i++) fputc(' ', out);
                fprintf(out, "XASource D:\\%s\n", source);
            } else {
                for (i = 0; i < indent + 2; i++) fputc(' ', out);
                fprintf(out, "XAFileAttributes Form1 Data\n");
                for (i = 0; i < indent + 2; i++) fputc(' ', out);
                fprintf(out, "Source D:\\%s\n", source);
            }

            for (i = 0; i < indent; i++) fputc(' ', out);
            fprintf(out, "EndFile\n");
        }
    }

    /* Finalize Primary Data track structure declarations */
    fprintf(out, "      EndHierarchy\n    EndPrimaryVolume\n  EndVolume\n");
    fprintf(out, "  PostGap 150\nEndTrack\n\n");

    /* Pass 3: Process trailing CD-DA waveform Red-Book audio tracks */
    No_Audio_Track=1;
	
	fseek(in, 0, SEEK_SET);
    while (fgets(line, sizeof(line), in)) {
        if (strstr(line, "<track type=\"audio\"")) {
            char source[256];
            get_attr_value(line, "source=", source);
            
            for (i = 0; source[i] != '\0'; i++) {
                if (source[i] == '/') source[i] = '\\';
            }
            
            fprintf(out, "Track Audio\n  Empty 150\n  Source D:\\%s\nEndTrack\n\n", source);

			No_Audio_Track=0;
            
            //break;
        }
    }
	
	if (No_Audio_Track)
	{
		fprintf(out, "LeadOut XA\n  Empty 150\nEndTrack\n\n");
		fprintf(out, "EndDisc\n");
	} else 
	
	{
	fprintf(out, "LeadOut Audio\n  Empty 150\nEndTrack\n\n");
    fprintf(out, "EndDisc\n");
	}

    fclose(in);
    fclose(out);
    printf("Successfully completed XML to CTI transformation map!\n");
    return 0;
}
