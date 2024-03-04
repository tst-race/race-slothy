from __future__ import unicode_literals

from django.db import models


class Document(models.Model):
    description = models.CharField(max_length=255, blank=True)
    document = models.FileField()
    uploaded_at = models.DateTimeField(auto_now_add=True)


class EncryptedFile(models.Model):
    description = models.CharField(max_length=255, blank=True)
    document = models.FileField()
    uploaded_at = models.DateTimeField(auto_now_add=True)
    pushed = models.BooleanField()


class Shard(models.Model):
    description = models.CharField(max_length=255, blank=True)
    document = models.FileField()
    uploaded_at = models.DateTimeField(auto_now_add=True)
    pushed = models.BooleanField()


class RaceNodes(models.Model):
    json = models.TextField()


class ServerNodes(models.Model):
    json = models.TextField()


class Dependencies(models.Model):
    json = models.TextField()


class Registries(models.Model):
    json = models.TextField()
