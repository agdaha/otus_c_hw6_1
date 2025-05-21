#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


typedef struct
{
  gchar *name;           
  gchar *path;           
  gboolean is_dir; 
  guint64 size;          
  time_t mtime;          
} FileInfo;

void file_info_free(FileInfo *info)
{
  if (info)
  {
    g_free(info->name);
    g_free(info->path);
    g_free(info);      
  }
}

static gint sort_files(gconstpointer a, gconstpointer b)
{
  const FileInfo *file_a = (FileInfo *)a;
  const FileInfo *file_b = (FileInfo *)b;

  if (file_a->is_dir != file_b->is_dir)
  {
    return file_b->is_dir - file_a->is_dir;
  }
  return g_ascii_strncasecmp(file_a->path, file_b->path, strlen(file_a->path));
}


GList *scan_dir(const gchar *path, GList *file_list)
{
  DIR *dir;             
  struct dirent *entry; 
  struct stat stat_buf; 

  if (!(dir = opendir(path)))
  {
    return file_list; 
  }

  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
      continue; 
    }

    gchar *full_path = g_build_filename(path, entry->d_name, NULL); 
    if (stat(full_path, &stat_buf) == -1)
    {                    
      g_free(full_path); 
      continue;
    }

    FileInfo *info = g_new0(FileInfo, 1);           
    info->name = g_strdup(entry->d_name);           
    info->path = g_strdup(full_path);               
    info->mtime = stat_buf.st_mtime;                
    info->is_dir = S_ISDIR(stat_buf.st_mode);
    info->size = stat_buf.st_size;                 

    file_list = g_list_append(file_list, info); 

    if (info->is_dir)
    { 
      file_list = scan_dir(full_path, file_list);
    }

    g_free(full_path);
  }

  closedir(dir); 
  return file_list;
}

void free_file_list(GList *file_list)
{
  GList *iter; 
  for (iter = file_list; iter != NULL; iter = iter->next)
  {
    file_info_free((FileInfo *)iter->data); 
  }
  g_list_free(file_list); 
}


void fill_tree_store(GtkTreeStore *store, GList *file_list, const gchar *base_path)
{
  file_list = g_list_sort(file_list, sort_files);
  GtkTreeIter iter; 

  GHashTable *path_hash = g_hash_table_new(g_str_hash, g_str_equal); 
  GList *iter_list;                                                  

  char buff[100];

  for (iter_list = file_list; iter_list != NULL; iter_list = iter_list->next)
  {
    FileInfo *info = (FileInfo *)iter_list->data;        
    gchar *parent_path = g_path_get_dirname(info->path); 

    if (strcmp(parent_path, base_path) == 0)
    {                                            
      gtk_tree_store_append(store, &iter, NULL); 
    }
    else
    {
      GtkTreeIter *parent_iter = g_hash_table_lookup(path_hash, parent_path); 
      if (parent_iter)
      {
        gtk_tree_store_append(store, &iter, parent_iter); 
      }
      else
      {
        gtk_tree_store_append(store, &iter, NULL); 
      }
    }
    strftime(buff, sizeof(buff), "%d.%m.%Y %T", gmtime(&(info->mtime)));
    gtk_tree_store_set(store, &iter,
                       0, info->is_dir ? "📁" : "📄",
                       1, info->name,
                       2, info->size,
                       3, buff,
                       -1); 

    g_hash_table_insert(path_hash, g_strdup(info->path), gtk_tree_iter_copy(&iter));
    g_free(parent_path); 
  }

  g_hash_table_destroy(path_hash); 
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "File Explorer");
  gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GtkTreeStore *store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING);
  GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1, "", renderer,"text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1, "Name", renderer,"text", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1, "Size", renderer,"text", 2, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1, "DateTimeMod", renderer,"text", 3, NULL);

  gchar *current_dir = g_get_current_dir();           
  GList *file_list = NULL;                           
  file_list = scan_dir(current_dir, file_list); 

  fill_tree_store(store, file_list, current_dir); 
  free_file_list(file_list);                          
  g_free(current_dir);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
  gtk_container_add(GTK_CONTAINER(window), scrolled_window);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}