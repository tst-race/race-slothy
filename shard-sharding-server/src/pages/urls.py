from django.urls import re_path
from django.conf.urls.static import static

from django.conf import settings

from . import views

urlpatterns = [
    re_path(r"^$", views.index, name="index"),
    re_path(r"^upload/", views.upload, name="upload"),
    # re_path(r"^shard/", views.shard, name="shard"),
    # re_path(r"^push/", views.push, name="push"),
    re_path(r"^deploy/simple", views.deploy_simple, name="deploy_simple"),
    re_path(r"^deploy/", views.deploy, name="deploy"),
    re_path(r"^randomize/", views.randomize, name="randomize"),
    re_path(r"^config/registries/", views.registries, name="registries"),
    re_path(r"^config/dependencies/", views.dependencies, name="dependencies"),
    re_path(r"^config/race_nodes/", views.race_nodes, name="race_nodes"),
    re_path(r"^config/server_nodes/", views.server_nodes, name="server_nodes"),
    re_path(r"^config/", views.config, name="config"),
    re_path(r"^index/", views.index, name="index"),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
