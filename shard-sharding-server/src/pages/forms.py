from django import forms


class UploadDepForm(forms.Form):
    filename = forms.FileField()
    uuid = forms.CharField(label="UUID", required=True, widget=forms.TextInput(attrs={"size": 32}))
    name = forms.CharField(label="Name", required=False, widget=forms.TextInput(attrs={"size": 32}))
    deptype = forms.CharField(
        label="Type", required=True, widget=forms.TextInput(attrs={"size": 32})
    )
    stability = forms.FloatField(
        label="Stability",
        min_value=0.0,
        max_value=10.0,
        required=True,
        widget=forms.TextInput(attrs={"size": 5}),
    )
    speed = forms.FloatField(
        label="Speed",
        min_value=0.0,
        max_value=10.0,
        required=True,
        widget=forms.TextInput(attrs={"size": 5}),
    )
    covertness = forms.FloatField(
        label="Covertness",
        min_value=0.0,
        max_value=10.0,
        required=True,
        widget=forms.TextInput(attrs={"size": 5}),
    )
    cost = forms.FloatField(
        label="Cost",
        min_value=0.0,
        max_value=10.0,
        required=True,
        widget=forms.TextInput(attrs={"size": 5}),
    )
