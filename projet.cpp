#include "vec.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include <limits>
#include <math.h>
#include <iostream>
#include <algorithm>


using namespace std;

const float inf= std::numeric_limits<float>::infinity();

struct Sphere
{
    Point c; //centre
    int r; //rayon
    Color col; //couleur
};

struct Plan
{
    Point a; //point
    Vector n; //normal passant par a
    Color col; //couleur plan
};

struct Lumiere
{
    Vector dirL;
    Color col;
};

struct Hit
{
    float t;        // position sur le rayon, ou inf s'il n'y a pas d'intersection
    Point p;        // position du point, s'il existe
    Vector n;       // normale du point d'intersection, s'il existe
    Color color;    // couleur du point d'intersection, s'il existe

    Hit( ) : t(inf), p(), n(), color() {}     // pas d'intersection
    Hit(const float &x, const Point &point, const Vector &norm, const Color &c)
    {
        t=x; p=point; n=norm; color=c;
    }
};

struct Scene
{
    std::vector<Sphere> spheres;
    Plan plan;
    std::vector<Lumiere> lums;
};

float intersect_sphere (const Point &c, const float &r, const Point &o, const Vector &d)
{
    float a = dot(d,d);
    float b = 2*dot(d, Vector(c,o));
    float k = dot(Vector(c,o), Vector(c,o))- r*r;
    float delta = b*b-4*a*k;

    if(delta>=0)
    {
        float t1 = (-b - sqrt(b*b-4*a*k))/(2*a);
        float t2 = (-b + sqrt(b*b-4*a*k))/(2*a);

        if(t1<0&&t2>=0)
            return t2;
        if(t2<0&&t1>=0)
            return t1;
        if(t1<0&&t2<0)
            return inf;
        if(t1>=0&&t2>=0)
        {
            if(t1<t2)
                return t1;
            return t2;
        }
    }
    return inf;
}

Hit intersect_sphere_hit(const Sphere s, const Point &o, const Vector &d)
{
    float t = intersect_sphere(s.c, s.r, o, d);
    if(t<0) return {};
    return Hit(t,o, Vector(o+ t*d),s.col);
}

Hit intersect_spheres_hit(const Scene &scene, const Point &o, const Vector &d)
{
    Hit plus_proche;
    Hit inter;
    plus_proche.t= inf;

    for(const auto& sphere : scene.spheres)
    {
        inter = intersect_sphere_hit(sphere,o,d);
        if(inter.t<plus_proche.t)
        {
            plus_proche = inter;
        }
    }
    return plus_proche;
}

float intersect_spheres(const std::vector<Sphere>& spheres, const Point &o, const Vector &d)
{
    float tmin = intersect_sphere(spheres[0].c,spheres[0].r, o, d);;
    float aux;

    for(int i=1; i< spheres.size(); i++)
    {
        aux = intersect_sphere(spheres[i].c, spheres[i].r, o, d);
        if (aux<tmin)
        {
            tmin = aux;
        }
    }
    return tmin;
}

Hit intersect_plan(const Plan &p, const Point& o, const Vector& d)
{
    float t = dot(p.n,Vector(o,p.a))/dot(p.n,d);
    if (t<0) return {};
    return Hit(t, o, p.n, p.col);
}

Hit intersect_plan_hit(const Scene& scene, const Point& o, const Vector& d )
{
    Hit plus_proche;
    plus_proche.t = inf;
    Hit h= intersect_plan(scene.plan, o, d);//, plus_proche.t);
    if(h.t<plus_proche.t && h.t>0)
        plus_proche= h;

    return plus_proche;
}


//fonction du cours modifié
Hit intersect(const Scene& scene, const Point& o, const Vector& d )
{
    Hit plus_proche;
    Hit h;
    plus_proche.t= 0;
    float t =0;
    for(const auto &sphere : scene.spheres)
    {
        t = intersect_sphere(sphere.c, sphere.r, o, d);
        std::cout<<"c "<<sphere.c<<endl;
        std::cout<<"r "<<sphere.r<<endl;
        std::cout<<"t "<<t<<endl;
    }

    return plus_proche;
}

Color soleil(const Scene& scene, const Color &colP, const Vector &n) // direction lum, coul lum, coul p, norm p
{
    Color sol = Black();
    for (int i=0; i<scene.lums.size(); i++)
    {
        float cos_theta = std::max((float)0, (float)dot(normalize(scene.lums[i].dirL), normalize(n)));
        sol = sol + scene.lums[i].col*colP*cos_theta;
    }
    return sol;
}

Color soleil_hit(const Scene& scene, const Hit &h) // direction lum, coul lum, coul p, norm p
{
    Color sol = Black();
    for (int i=0; i<scene.lums.size(); i++)
    {
        float cos_theta = std::max((float)0, (float)dot(normalize(scene.lums[i].dirL), normalize(h.n)));
        sol = sol + scene.lums[i].col*h.color*cos_theta;
    }
    return sol;
}

Color calculer_ombre_reflechie(const Scene& scene, const Hit& h)
{
    if (h.t != inf)
    {
        Color c = Black();
        float e = 0.001;
        Point o = h.p + e * h.n;
        float theta = 0.0;

        for (const auto& lumiere : scene.lums) {
            Vector l = lumiere.dirL;
            theta = std::max(float(0), dot(h.n, l));

            bool est_dans_ombre = false;
            for (const auto& sphere : scene.spheres) {
                if (intersect_sphere(sphere.c, sphere.r, o, l) != inf) {
                    est_dans_ombre = true;
                    break;
                }
            }
            if (!est_dans_ombre)// si c est dan l ombre
            {
                c = c + h.color * lumiere.col * theta;
            }
        }
        return c;
    }
    return Black();
}

// methode pour appliquer un effet de bloom à une image avec un filtre, apres la double boucle du main
void filtre_image(Image& image, const Color& colore, float nivBloom, float intensite)
{
    for (int y = 0; y < image.height(); y++)
    {
        for (int x = 0; x < image.width(); x++)
        {
            Color pix = image(x, y);
            float rapport_lum = (pix.r + pix.g + pix.b) / 3.0f;

            if (rapport_lum>nivBloom)
            {
                float norm_rapport = (rapport_lum - nivBloom) / (1.0f - nivBloom);
                Color col_effet = colore * norm_rapport * intensite;
                image(x, y) = pix + col_effet;
            }
            else {
                image(x, y) = pix;
            }
        }
    }
}

//fonction pour calculer le vector reflexion, extraite du cours
Vector reflection(const Vector& incident, const Vector& normal)
{
    return incident - 2 * dot(incident, normal) * normal;
}

//fonction pour creer un effet lucide sur les sphere quand elles sont sous lumiere
Color effetLucide1(const Scene& scene,const Hit& h, const Color& colSpecular, float intensiteRef, const Vector &d, const float concentration) {

    Vector l = normalize(scene.lums[0].dirL);
    Vector n = normalize(h.n);
    Vector reflex = reflection(d, n);
    float distance = length(Vector(h.p) - normalize(scene.lums[0].dirL));
    float cos_tetha = dot(l, reflex);
    if(cos_tetha>0)
    {
        float intensite = pow(cos_tetha, intensiteRef);
        intensite *= exp(-concentration * distance);
        return h.color + colSpecular * intensite;
    }
    else return h.color;

}

Color couleurCiel(const Vector& directionLumiere, const Color& couleur) {
    float angle = dot(directionLumiere, Vector(0, 1, 0));
    float rouge = couleur.r * (1.0f - angle); // Plus de rouge vers le bas
    float vert = couleur.g * (1.0f - angle);  // Plus de vert vers le bas
    float bleu = couleur.b * (1.0f - angle);  // Plus de bleu vers le bas
    return Color(rouge, vert, bleu);
}


Color couleurCielInterpole(const Vector& d, const Lumiere& l1, const Lumiere& l2) {
    float angle1 = dot(normalize(d), normalize(l1.dirL));
    float angle2 = dot(normalize(d), normalize(l2.dirL));

    if(angle2==0)
    {
        return l1.col;
    }

    else if (angle1==0 && angle2!=0)
    {
        return l2.col;
    }
    else{
            float angle = (angle1 + angle2)*1.8;
    Color couleurInterpolee = (1.0f - angle) * l1.col + angle * l2.col;
    return couleurInterpolee;
    }
}

Color couleurCielDeuxLum(const Lumiere& lumiereBasse, const Lumiere& lumiereHaute) {
    float angle = dot(lumiereBasse.dirL, lumiereHaute.dirL);
    Color couleurInterpolee = (1.0f - angle) * lumiereBasse.col + angle * lumiereHaute.col;
    return couleurInterpolee;
}


//scene avec ombre reflechie
int main( )
{
    Image imageJour(1024, 512);

    float ratioWH = (float)(imageJour.width())/(float)(imageJour.height());

    Sphere s1;
    s1.c = Point(-1,0,-3);
    s1.r = 1;
    s1.col=Red();

    Sphere s2;
    s2.c = Point(1,0,-3);
    s2.r = 1;
    s2.col=Blue()+Color(0,0.3,0) + Color(0.7,0,0);

    Sphere s3;
    s3.c = Point(-3,0,-3);
    s3.r = 1;
    s3.col=Yellow();

    Sphere s4;
    s4.c = Point(0,0,-2);
    s4.r = 1;
    s4.col=Yellow()+Color(0.2,0,0.5);

    Plan p;
    p.a = Point(0,-1, 0);
    p.n = Vector(0, 1, 0);
    p.col = Green()-Color(0,0.3,0) + Color(0.1,0,0.1);

    Lumiere lum1;
    lum1.dirL = Vector(1,0.5,0);
    lum1.col = Color(0.7,0.1,0.3);//Blue()- Color(0,0,0.2);//Red()+ Color(0,0,0.5)-Color(0.2,0,0);

    Lumiere lum2;
    lum2.dirL = Vector(-1,0.5,0);
    lum2.col = Color(0.2,0.1,0.5);//Blue()- Color(0,0,0.1);

    Scene scene;

    scene.spheres.push_back(s1);
    scene.spheres.push_back(s2);
    scene.spheres.push_back(s3);
    scene.spheres.push_back(s4);


    scene.plan=p;
    scene.lums.push_back(lum1);
    scene.lums.push_back(lum2);



    Point o= Point(0, 0, 0);

    for(int py = 0; py < imageJour.height(); py++) {
    for(int px = 0; px < imageJour.width(); px++) {
        Point o = Point(0, 0, 0);    // origine
        Point e = Point(((float)px) / ((float)imageJour.width()) * 2 - 1,
                        ((float)py) / ((float)imageJour.height()) * 2 - 1,
                        -1); // extremite


        e.x = e.x * ratioWH;
        Vector d = Vector(o, e);     // direction : extremite - origine

        Hit interScene = intersect_plan_hit(scene , o, d); //intersect(scene, o, d);
        interScene.p = o + interScene.t*d;
        Color couleur_finale = soleil(scene, interScene.color, interScene.n)+ calculer_ombre_reflechie(scene, interScene);//+effettoLucido(interScene,White(),1);//+effetNuitScene(scene, interScene,0.05,1);//+ calculer_reflexion(scene, interScene, 2);

        if(interScene.t!=inf)
        {
            imageJour(px,py) =couleur_finale;
        }

        float t1 = intersect_sphere(scene.spheres[0].c,scene.spheres[0].r,o,d);
        float t2 = intersect_sphere(scene.spheres[1].c,scene.spheres[1].r,o,d);
        float t3 = intersect_sphere(scene.spheres[2].c,scene.spheres[2].r,o,d);

        if(t1!=inf && t1<interScene.t && t1<t2)
        {
            Point interSphere = o + t1*d;
            imageJour(px, py) = soleil(scene,  scene.spheres[0].col, Vector(scene.spheres[0].c, interSphere));
        }

        if(t2!=inf&&t2<interScene.t&&t2<t1)
        {
            Point interSphere = o + t2*d;
            imageJour(px, py) = soleil(scene, scene.spheres[1].col, Vector(scene.spheres[1].c, interSphere));

        }

        if(t3!=inf&&t3<interScene.t&&t3<t1&&t3<t2)
        {
            Point interSphere = o + t3*d;
            imageJour(px, py) = soleil(scene, scene.spheres[2].col, Vector(scene.spheres[2].c, interSphere));
        }

        Hit inter_sphere_s4 = intersect_sphere_hit(scene.spheres[3], o,d);
        if(inter_sphere_s4.t !=inf)
        {
            imageJour(px, py) = soleil(scene, inter_sphere_s4.color, inter_sphere_s4.n);
        }

        if(t1==inf&&t2==inf&&t3==inf&&inter_sphere_s4.t==inf&&interScene.t==inf)
        {
            imageJour(px, py)=couleurCielInterpole(d, scene.lums[0], scene.lums[1]);
        }


    }}

    write_image_preview(imageJour, "images/image_soiree.png");
    filtre_image(imageJour,Blue(), 0.05, 7);

    write_image_preview(imageJour, "images/image_nuit.png");
    return 0;
}




